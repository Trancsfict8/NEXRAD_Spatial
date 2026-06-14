#include "StormAttributeManager.h"
#include "../Application/GlobalState.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Radar/Globe.h"
#include "../Radar/RadarCollection.h"
#include "../Radar/RadarDataHolder.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AStormAttributeManager::AStormAttributeManager()
{
    PrimaryActorTick.bCanEverTick = true;
    
    TvsMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TvsMeshComponent"));
    RootComponent = TvsMeshComponent;
    
    HailSmallMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HailSmallMeshComponent"));
    HailSmallMeshComponent->SetupAttachment(RootComponent);
    
    HailMediumMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HailMediumMeshComponent"));
    HailMediumMeshComponent->SetupAttachment(RootComponent);
    
    HailLargeMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HailLargeMeshComponent"));
    HailLargeMeshComponent->SetupAttachment(RootComponent);

    // Setup some default basic shapes (User can change these to actual nice icons later)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("StaticMesh'/Engine/BasicShapes/Cone.Cone'"));
    if (ConeMesh.Succeeded()) TvsMeshComponent->SetStaticMesh(ConeMesh.Object);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
    if (SphereMesh.Succeeded()) {
        HailSmallMeshComponent->SetStaticMesh(SphereMesh.Object);
        HailMediumMeshComponent->SetStaticMesh(SphereMesh.Object);
        HailLargeMeshComponent->SetStaticMesh(SphereMesh.Object);
    }

    // Initial scale for visualization is applied per-instance in UpdateMeshes
}

void AStormAttributeManager::BeginPlay()
{
    Super::BeginPlay();
    
    UMaterial* baseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (baseMat) {
        UMaterialInstanceDynamic* dynTvs = UMaterialInstanceDynamic::Create(baseMat, this);
        dynTvs->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.0f, 1.0f, 1.0f));
        TvsMeshComponent->SetMaterial(0, dynTvs);
        
        UMaterialInstanceDynamic* dynHailSmall = UMaterialInstanceDynamic::Create(baseMat, this);
        dynHailSmall->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
        HailSmallMeshComponent->SetMaterial(0, dynHailSmall);
        
        UMaterialInstanceDynamic* dynHailMed = UMaterialInstanceDynamic::Create(baseMat, this);
        dynHailMed->SetVectorParameterValue(TEXT("Color"), FLinearColor::Yellow);
        HailMediumMeshComponent->SetMaterial(0, dynHailMed);
        
        UMaterialInstanceDynamic* dynHailLarge = UMaterialInstanceDynamic::Create(baseMat, this);
        dynHailLarge->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
        HailLargeMeshComponent->SetMaterial(0, dynHailLarge);
    }
    
    TextMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/UnlitColorText.UnlitColorText"));
}

void AStormAttributeManager::EndPlay(const EEndPlayReason::Type endPlayReason)
{
    Super::EndPlay(endPlayReason);
}

void AStormAttributeManager::HideAllText()
{
    for (UTextRenderComponent* TextComp : TextComponents) {
        if (TextComp) TextComp->SetVisibility(false);
    }
}

void AStormAttributeManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    GlobalState* globalState = nullptr;
    if (GetWorld()->GetGameState<ARadarGameStateBase>()) {
        globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
    }
    
    if (!globalState || !globalState->downloadData) {
        TvsMeshComponent->ClearInstances();
        HailSmallMeshComponent->ClearInstances();
        HailMediumMeshComponent->ClearInstances();
        HailLargeMeshComponent->ClearInstances();
        HideAllText();
        currentAttributes.Empty();
        return;
    }
    
    globe = globalState->globe;
    if (!globe) return;
    
    // Check if site changed
    FString currentSite = FString(globalState->downloadSiteId.c_str());
    if (currentSite != lastSiteId) {
        lastSiteId = currentSite;
        currentAttributes.Empty();
        lastFetchTime = fetchInterval; // force fetch immediately on site change
    }
    
    if (!globalState->showLevel3StormAttributes) {
        TvsMeshComponent->ClearInstances();
        HailSmallMeshComponent->ClearInstances();
        HailMediumMeshComponent->ClearInstances();
        HailLargeMeshComponent->ClearInstances();
        HideAllText();
        lastFetchTime = fetchInterval; // force fetch immediately when toggled back on
        return;
    }
    
    lastFetchTime += DeltaTime;
    if (lastFetchTime > fetchInterval) {
        lastFetchTime = 0;
        FetchStormAttributes();
    }
    
    UpdateMeshes();
}

void AStormAttributeManager::FetchStormAttributes()
{
    if (lastSiteId.IsEmpty()) return;

    FString url = TEXT("https://mesonet.agron.iastate.edu/geojson/nexrad_attr.geojson");
    
    UE_LOG(LogTemp, Warning, TEXT("StormAttributeManager fetching attributes: %s"), *url);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(url);
    Request->SetVerb("GET");
    Request->SetHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 OpenStormVR/1.0");
    Request->OnProcessRequestComplete().BindUObject(this, &AStormAttributeManager::OnAttributesFetched);
    Request->ProcessRequest();
}

void AStormAttributeManager::OnAttributesFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid()) {
        UE_LOG(LogTemp, Warning, TEXT("StormAttributeManager failed to fetch geojson."));
        return;
    }

    FString Content = Response->GetContentAsString();
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* Features;
        if (JsonObject->TryGetArrayField(TEXT("features"), Features))
        {
            FString siteStr = lastSiteId;
            if (siteStr.Len() == 4) siteStr = siteStr.Right(3);
            siteStr = siteStr.ToUpper();
            
            TArray<FStormAttr> newAttrs;

            for (TSharedPtr<FJsonValue> FeatureVal : *Features)
            {
                TSharedPtr<FJsonObject> FeatureObj = FeatureVal->AsObject();
                if (!FeatureObj.IsValid()) continue;

                TSharedPtr<FJsonObject> PropsObj = FeatureObj->GetObjectField(TEXT("properties"));
                TSharedPtr<FJsonObject> GeomObj = FeatureObj->GetObjectField(TEXT("geometry"));
                
                if (!PropsObj.IsValid() || !GeomObj.IsValid()) continue;

                FString nexrad = PropsObj->GetStringField(TEXT("nexrad"));
                if (nexrad != siteStr) continue;

                FString tvs = PropsObj->GetStringField(TEXT("tvs"));
                float poh = PropsObj->GetNumberField(TEXT("poh"));
                
                if (tvs == TEXT("NONE") && poh < 50.0f) continue; // Not a TVS, and low hail prob

                const TArray<TSharedPtr<FJsonValue>>* Coords;
                if (GeomObj->TryGetArrayField(TEXT("coordinates"), Coords) && Coords->Num() >= 2)
                {
                    FStormAttr attr;
                    attr.lon = (*Coords)[0]->AsNumber();
                    attr.lat = (*Coords)[1]->AsNumber();
                    attr.maxSize = PropsObj->GetNumberField(TEXT("max_size"));
                    
                    if (tvs != TEXT("NONE")) {
                        attr.type = TEXT("TVS");
                        newAttrs.Add(attr);
                    } 
                    if (poh >= 50.0f) {
                        FStormAttr hailAttr = attr;
                        hailAttr.type = TEXT("HAIL");
                        newAttrs.Add(hailAttr);
                    }
                }
            }
            
            UE_LOG(LogTemp, Warning, TEXT("StormAttributeManager found %d active attributes for site %s"), newAttrs.Num(), *siteStr);

            FScopeLock lock(&attributesMutex);
            currentAttributes = newAttrs;
        }
    }
}

void AStormAttributeManager::UpdateMeshes()
{
    if (!globe) return;
    
    FScopeLock lock(&attributesMutex);
    
    TvsMeshComponent->ClearInstances();
    HailSmallMeshComponent->ClearInstances();
    HailMediumMeshComponent->ClearInstances();
    HailLargeMeshComponent->ClearInstances();
    HideAllText();

    GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
    if (!globalState || !globalState->refRadarCollection) return;
    
    double siteLat = 0, siteLon = 0;
    RadarDataHolder* holder = globalState->refRadarCollection->GetCurrentRadarData();
    if (holder && holder->radarData) {
        siteLat = holder->radarData->stats.latitude;
        siteLon = holder->radarData->stats.longitude;
    }

    if (siteLat == 0 && siteLon == 0) return; // No radar data loaded to reference

    int textIndex = 0;
    
    for (const FStormAttr& attr : currentAttributes) {
        if (attr.type == TEXT("TVS")) {
            // Position TVS higher so it hovers above any hail sphere
            SimpleVector3<double> loc = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, 15000 * globalState->elevationExaggeration);
            FTransform transform;
            transform.SetLocation(FVector(loc.x, loc.y, loc.z));
            
            // Point the cone downwards by flipping pitch, and stretch it slightly
            // Using FQuat to set rotation properly. In VR map, downward relative to globe requires orienting to globe center.
            // Since basic shapes might just point Z up, we can invert pitch/roll or just scale Z by -1
            transform.SetRotation(FQuat(FRotator(180.0f, 0.0f, 0.0f)));
            transform.SetScale3D(FVector(0.5f, 0.5f, 0.8f)); 
            
            TvsMeshComponent->AddInstance(transform);
        } else {
            // HAIL
            // Position 40k feet high (12192 meters) so it's above the storm cells
            SimpleVector3<double> loc = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, 12192 * globalState->elevationExaggeration);

            FTransform transform;
            transform.SetLocation(FVector(loc.x, loc.y, loc.z));
            transform.SetScale3D(FVector(0.5f)); // 0.5 scale * 100 unit sphere * 100m/unit = 5km wide spheres

            if (attr.maxSize >= 2.0f) {
                HailLargeMeshComponent->AddInstance(transform);
            } else if (attr.maxSize >= 1.0f) {
                HailMediumMeshComponent->AddInstance(transform);
            } else {
                HailSmallMeshComponent->AddInstance(transform);
            }
            
            // Add Text for Hail Size
            if (TextComponents.Num() <= textIndex) {
                UTextRenderComponent* NewText = NewObject<UTextRenderComponent>(this);
                NewText->RegisterComponent();
                NewText->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
                NewText->SetHorizontalAlignment(EHTA_Center);
                if (TextMaterial) NewText->SetTextMaterial(TextMaterial);
                NewText->SetWorldSize(30.0f); // 30 Unreal Units (equivalent to 3km in VR map scale, slightly smaller than the 5km sphere)
                TextComponents.Add(NewText);
            }
            
            UTextRenderComponent* TextComp = TextComponents[textIndex];
            TextComp->SetVisibility(true);
            TextComp->SetText(FText::FromString(FString::Printf(TEXT("%.2f\""), attr.maxSize)));
            
            SimpleVector3<double> locText = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, (12192 + 3000) * globalState->elevationExaggeration);
            TextComp->SetWorldLocation(FVector(locText.x, locText.y, locText.z)); // Place 3km higher than the sphere
            
            // Billboarding: face the camera
            if (GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager) {
                FVector CameraLoc = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
                FRotator FaceRot = (CameraLoc - TextComp->GetComponentLocation()).Rotation();
                TextComp->SetWorldRotation(FaceRot);
            }
            
            textIndex++;
        }
    }
}
