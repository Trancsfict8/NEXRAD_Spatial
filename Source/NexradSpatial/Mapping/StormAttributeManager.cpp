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

    TrackMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TrackMeshComponent"));
    TrackMeshComponent->SetupAttachment(RootComponent);

    TrackMarkerMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TrackMarkerMeshComponent"));
    TrackMarkerMeshComponent->SetupAttachment(RootComponent);

    // Setup some default basic shapes (User can change these to actual nice icons later)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("StaticMesh'/Engine/BasicShapes/Cone.Cone'"));
    if (ConeMesh.Succeeded()) TvsMeshComponent->SetStaticMesh(ConeMesh.Object);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
    if (SphereMesh.Succeeded()) {
        HailSmallMeshComponent->SetStaticMesh(SphereMesh.Object);
        HailMediumMeshComponent->SetStaticMesh(SphereMesh.Object);
        HailLargeMeshComponent->SetStaticMesh(SphereMesh.Object);
        TrackMarkerMeshComponent->SetStaticMesh(SphereMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("StaticMesh'/Engine/BasicShapes/Cylinder.Cylinder'"));
    if (CylinderMesh.Succeeded()) TrackMeshComponent->SetStaticMesh(CylinderMesh.Object);

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

        UMaterialInstanceDynamic* dynTrack = UMaterialInstanceDynamic::Create(baseMat, this);
        dynTrack->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
        TrackMeshComponent->SetMaterial(0, dynTrack);

        UMaterialInstanceDynamic* dynTrackMarker = UMaterialInstanceDynamic::Create(baseMat, this);
        dynTrackMarker->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
        TrackMarkerMeshComponent->SetMaterial(0, dynTrackMarker);
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
    for (UTextRenderComponent* TopComp : EchoTopTextComponents) {
        if (TopComp) TopComp->SetVisibility(false);
    }
}

void AStormAttributeManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    GlobalState* globalState = nullptr;
    if (GetWorld()->GetGameState<ARadarGameStateBase>()) {
        globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
    }
    
    bool wantsAny = globalState->showLevel3StormAttributes || globalState->showLevel3StormTracks;

    if (!globalState || !globalState->downloadData || !wantsAny) {
        TvsMeshComponent->ClearInstances();
        HailSmallMeshComponent->ClearInstances();
        HailMediumMeshComponent->ClearInstances();
        HailLargeMeshComponent->ClearInstances();
        TrackMeshComponent->ClearInstances();
        TrackMarkerMeshComponent->ClearInstances();
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
    
    // Global attribute toggling is handled in UpdateMeshes to allow dynamic updates
    // without refetching data constantly when toggling specific layers.
    
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
                float drct = PropsObj->GetNumberField(TEXT("drct"));
                float sknt = PropsObj->GetNumberField(TEXT("sknt"));
                
                bool isTVS = (tvs != TEXT("NONE"));
                bool isHail = (poh >= 50.0f);
                bool hasTrack = (sknt > 0);

                if (!isTVS && !isHail && !hasTrack) continue;

                const TArray<TSharedPtr<FJsonValue>>* Coords;
                if (GeomObj->TryGetArrayField(TEXT("coordinates"), Coords) && Coords->Num() >= 2)
                {
                    FStormAttr attr;
                    attr.lon = (*Coords)[0]->AsNumber();
                    attr.lat = (*Coords)[1]->AsNumber();
                    attr.maxSize = PropsObj->GetNumberField(TEXT("max_size"));
                    attr.drct = drct;
                    attr.sknt = sknt;
                    attr.storm_id = PropsObj->GetStringField(TEXT("storm_id"));
                    attr.top = PropsObj->GetNumberField(TEXT("top"));
                    
                    if (isTVS) {
                        attr.type = TEXT("TVS");
                        newAttrs.Add(attr);
                    } 
                    if (isHail && attr.maxSize >= 0.25f) {
                        FStormAttr hailAttr = attr;
                        hailAttr.type = TEXT("HAIL");
                        newAttrs.Add(hailAttr);
                    }
                    if (hasTrack) {
                        FStormAttr trackAttr = attr;
                        trackAttr.type = TEXT("TRACK");
                        newAttrs.Add(trackAttr);
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
    TrackMeshComponent->ClearInstances();
    TrackMarkerMeshComponent->ClearInstances();
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
        if (attr.type == TEXT("TVS") && globalState->showLevel3StormAttributes) {
            // Position TVS higher so it hovers above any hail sphere
            SimpleVector3<double> loc = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, 15000 * globalState->elevationExaggeration);
            FTransform transform;
            transform.SetLocation(FVector(loc.x, loc.y, loc.z));
            
            transform.SetRotation(FQuat(FRotator(180.0f, 0.0f, 0.0f)));
            transform.SetScale3D(FVector(0.5f, 0.5f, 0.8f)); 
            
            TvsMeshComponent->AddInstance(transform);
        } else if (attr.type == TEXT("HAIL") && globalState->showLevel3StormAttributes) {
            // HAIL
            // Use attr.top * 1000.0f * 0.3048f as altitude in meters, fallback to 12192 (40k ft) if missing
            float altitudeMeters = (attr.top > 0.0f) ? (attr.top * 1000.0f * 0.3048f) : 12192.0f;
            
            SimpleVector3<double> loc = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, altitudeMeters * globalState->elevationExaggeration);

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
                NewText->SetWorldSize(30.0f); // 30 Unreal Units (equivalent to 3km in VR map scale)
                TextComponents.Add(NewText);
            }
            
            if (EchoTopTextComponents.Num() <= textIndex) {
                UTextRenderComponent* NewTopText = NewObject<UTextRenderComponent>(this);
                NewTopText->RegisterComponent();
                NewTopText->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
                NewTopText->SetHorizontalAlignment(EHTA_Center);
                if (TextMaterial) NewTopText->SetTextMaterial(TextMaterial);
                NewTopText->SetWorldSize(15.0f); // 15 Unreal Units (smaller text for Echo Top)
                EchoTopTextComponents.Add(NewTopText);
            }
            
            UTextRenderComponent* TextComp = TextComponents[textIndex];
            TextComp->SetVisibility(true);
            TextComp->SetText(FText::FromString(FString::Printf(TEXT("%.2f\""), attr.maxSize)));
            
            UTextRenderComponent* TopComp = EchoTopTextComponents[textIndex];
            TopComp->SetVisibility(true);
            TopComp->SetText(FText::FromString(FString::Printf(TEXT("%.0f ft AGL"), attr.top * 1000.0f)));
            
            // Hail size text higher up
            SimpleVector3<double> locText = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, (altitudeMeters + 4500) * globalState->elevationExaggeration);
            TextComp->SetWorldLocation(FVector(locText.x, locText.y, locText.z)); 
            
            // Echo top text slightly lower than hail size, but above sphere
            SimpleVector3<double> locTopText = globalState->globe->GetPointScaledDegrees(attr.lat, attr.lon, (altitudeMeters + 2800) * globalState->elevationExaggeration);
            TopComp->SetWorldLocation(FVector(locTopText.x, locTopText.y, locTopText.z)); 
            
            // Billboarding: face the camera
            if (GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager) {
                FVector CameraLoc = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
                
                FRotator FaceRot = (CameraLoc - TextComp->GetComponentLocation()).Rotation();
                TextComp->SetWorldRotation(FaceRot);
                
                FRotator FaceRotTop = (CameraLoc - TopComp->GetComponentLocation()).Rotation();
                TopComp->SetWorldRotation(FaceRotTop);
            }
            
            textIndex++;
        } else if (attr.type == TEXT("TRACK") && globalState->showLevel3StormTracks) {
            double headingDeg = attr.drct + 180.0; // direction it's heading
            double headingRad = FMath::DegreesToRadians(headingDeg);
            double distanceKmPerHour = attr.sknt * 1.852;
            double earthRadiusKm = 6371.0;
            
            // Draw 4 segments for 15, 30, 45, 60 minutes
            for (int step = 0; step < 4; step++) {
                double startDistKm = distanceKmPerHour * (step * 0.25);
                double endDistKm = distanceKmPerHour * ((step + 1) * 0.25);
                
                double startLat = attr.lat + (startDistKm * cos(headingRad) / earthRadiusKm) * (180.0 / PI);
                double startLon = attr.lon + (startDistKm * sin(headingRad) / (earthRadiusKm * cos(FMath::DegreesToRadians(attr.lat)))) * (180.0 / PI);
                
                double endLat = attr.lat + (endDistKm * cos(headingRad) / earthRadiusKm) * (180.0 / PI);
                double endLon = attr.lon + (endDistKm * sin(headingRad) / (earthRadiusKm * cos(FMath::DegreesToRadians(attr.lat)))) * (180.0 / PI);
                
                // 9000 ft = 2743.2 meters AGL (3 times higher)
                SimpleVector3<double> locStart = globalState->globe->GetPointScaledDegrees(startLat, startLon, 2743.2 * globalState->elevationExaggeration);
                SimpleVector3<double> locEnd = globalState->globe->GetPointScaledDegrees(endLat, endLon, 2743.2 * globalState->elevationExaggeration);
                
                FVector StartVec(locStart.x, locStart.y, locStart.z);
                FVector EndVec(locEnd.x, locEnd.y, locEnd.z);
                
                FVector Delta = EndVec - StartVec;
                float Length = Delta.Size();
                if (Length < 0.1f) continue;
                
                FTransform transform;
                transform.SetLocation(StartVec + Delta * 0.5f);
                
                FRotator Rot = Delta.Rotation();
                Rot.Pitch -= 90.0f; // Align cylinder Z to face along X (forward)
                transform.SetRotation(Rot.Quaternion());
                
                // Half as thick
                transform.SetScale3D(FVector(0.025f, 0.025f, Length / 100.0f));
                
                TrackMeshComponent->AddInstance(transform);

                // Add marker
                FTransform markerTransform;
                markerTransform.SetLocation(EndVec);
                markerTransform.SetScale3D(FVector(0.1f)); // Slightly wider than the line (0.1 scale vs 0.025)
                TrackMarkerMeshComponent->AddInstance(markerTransform);
            }
        }
    }
}
