// Fill out your copyright notice in the Description page of Project Settings.


#include "RadarViewPawn.h"
#include "RadarVolumeRender.h"
#include "RadarGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h" 
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "../Radar/SimpleVector3.h"
#include "../Radar/Globe.h"
#include "../Radar/NexradSites/NexradSites.h"
#include "../Mapping/LocationMarker.h"
#include "../UI/ClickableInterface.h"
#include "../UI/ImGuiController.h"
#include "../UI/Slate/SlateUI.h"
#include "../UI/Slate/SVRMenuWidget.h"
#include "Engine/Engine.h"
#include "MotionControllerComponent.h"
#include "XRDeviceVisualizationComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"



// Sets default values
ARadarViewPawn::ARadarViewPawn()
{
	//Setup Rootcomponent
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ViewportMesh"));
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	
	UStaticMesh * playerMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Game/Meshes/inverted_cube.inverted_cube'")).Object;
	UMaterial * material = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/RadarVolumeMaterial.RadarVolumeMaterial'")).Object;
	


	meshComponent->SetStaticMesh(playerMesh);
	meshComponent->SetMaterial(0, material);
	meshComponent->TranslucencySortPriority = 1;
	meshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	camera->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	meshComponent->AttachToComponent(camera, FAttachmentTransformRules::KeepRelativeTransform);
	
	// Left motion controller - tracks position/rotation of left hand
	leftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	leftController->MotionSource = FName("Left");
	leftController->SetupAttachment(RootComponent);

	// Visual mesh for left controller (rendered by XR runtime / OpenXR device model)
	leftControllerVisual = CreateDefaultSubobject<UXRDeviceVisualizationComponent>(TEXT("LeftControllerVisual"));
	leftControllerVisual->SetupAttachment(leftController);
	leftControllerVisual->SetIsVisualizationActive(true);
	leftControllerVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Right motion controller
	rightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	rightController->MotionSource = FName("Right");
	rightController->SetupAttachment(RootComponent);

	// Visual mesh for right controller
	rightControllerVisual = CreateDefaultSubobject<UXRDeviceVisualizationComponent>(TEXT("RightControllerVisual"));
	rightControllerVisual->SetupAttachment(rightController);
	rightControllerVisual->SetIsVisualizationActive(true);
	rightControllerVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void ARadarViewPawn::BeginPlay()
{
	Super::BeginPlay();
	
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);

	// VR Menu panel
	vrMenuWidget = NewObject<UWidgetComponent>(this, TEXT("VRMenuWidget_Dyn"));
	vrMenuWidget->SetupAttachment(leftController);
	vrMenuWidget->RegisterComponent();
	vrMenuWidget->SetDrawSize(FVector2D(1200, 1000));
	vrMenuWidget->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.04f));
	vrMenuWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	vrMenuWidget->SetRelativeRotation(FRotator(60.0f, 180.0f, 0.0f));
	vrMenuWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	vrMenuWidget->SetCollisionResponseToAllChannels(ECR_Ignore);
	vrMenuWidget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	vrMenuWidget->SetTwoSided(true);
	vrMenuWidget->SetTickWhenOffscreen(true);
	vrMenuWidget->SetWindowFocusable(true);
	vrMenuWidget->SetSlateWidget(SNew(SVRMenuWidget));

	// Laser pointer
	widgetInteraction = NewObject<UWidgetInteractionComponent>(this, TEXT("WidgetInteraction_Dyn"));
	widgetInteraction->SetupAttachment(rightController);
	widgetInteraction->RegisterComponent();
	widgetInteraction->TraceChannel = ECollisionChannel::ECC_Visibility;
	widgetInteraction->InteractionSource = EWidgetInteractionSource::World;

	inspectorMesh = NewObject<UStaticMeshComponent>(this, TEXT("InspectorMesh"));
	inspectorMesh->SetupAttachment(RootComponent);
	inspectorMesh->SetStaticMesh(Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"))));
	inspectorMesh->RegisterComponent();
	inspectorMesh->SetWorldScale3D(FVector(0.02f));
	inspectorMesh->SetVisibility(false);
	inspectorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	inspectorText = NewObject<UTextRenderComponent>(this, TEXT("InspectorText"));
	inspectorText->SetupAttachment(inspectorMesh);
	inspectorText->RegisterComponent();
	inspectorText->SetRelativeLocation(FVector(0, 0, 100));
	inspectorText->SetVisibility(false);
	inspectorText->SetTextRenderColor(FColor::White);
	inspectorText->SetWorldSize(150.0f);
	inspectorText->SetHorizontalAlignment(EHTA_Center);
	
	inspectorTextShadow = NewObject<UTextRenderComponent>(this, TEXT("InspectorTextShadow"));
	inspectorTextShadow->SetupAttachment(inspectorText);
	inspectorTextShadow->RegisterComponent();
	inspectorTextShadow->SetRelativeLocation(FVector(-1.5f, 1.5f, -1.5f));
	inspectorTextShadow->SetVisibility(false);
	inspectorTextShadow->SetTextRenderColor(FColor::Black);
	inspectorTextShadow->SetWorldSize(160.0f);
	inspectorTextShadow->SetHorizontalAlignment(EHTA_Center);
	
	inspectorTooltip = NewObject<UTextRenderComponent>(this, TEXT("InspectorTooltip"));
	inspectorTooltip->SetupAttachment(inspectorText);
	inspectorTooltip->RegisterComponent();
	inspectorTooltip->SetRelativeLocation(FVector(0, 0, -125.0f));
	inspectorTooltip->SetVisibility(false);
	inspectorTooltip->SetTextRenderColor(FColor(200, 200, 200));
	inspectorTooltip->SetWorldSize(60.0f);
	inspectorTooltip->SetHorizontalAlignment(EHTA_Center);
	inspectorTooltip->SetText(FText::FromString(TEXT("Hold Left 'X' + Right Joystick\nto adjust distance")));

	inspectorTooltipShadow = NewObject<UTextRenderComponent>(this, TEXT("InspectorTooltipShadow"));
	inspectorTooltipShadow->SetupAttachment(inspectorTooltip);
	inspectorTooltipShadow->RegisterComponent();
	inspectorTooltipShadow->SetRelativeLocation(FVector(-1.0f, 1.0f, -1.0f));
	inspectorTooltipShadow->SetVisibility(false);
	inspectorTooltipShadow->SetTextRenderColor(FColor::Black);
	inspectorTooltipShadow->SetWorldSize(65.0f);
	inspectorTooltipShadow->SetHorizontalAlignment(EHTA_Center);
	inspectorTooltipShadow->SetText(FText::FromString(TEXT("Hold Left 'X' + Right Joystick\nto adjust distance")));
	widgetInteraction->InteractionDistance = 100000.0f;
	widgetInteraction->bShowDebug = false;
	widgetInteraction->SetRelativeLocation(FVector(3.0f, 0.0f, -4.0f));
	widgetInteraction->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));

	// Create physical laser pointer dynamically at runtime to avoid UE5 C++ constructor corruption
	UStaticMeshComponent* dynamicLaser = NewObject<UStaticMeshComponent>(this, TEXT("DynamicLaserMesh"));
	dynamicLaser->SetupAttachment(widgetInteraction);
	dynamicLaser->RegisterComponent();
	UStaticMesh* cylMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (cylMesh) {
		dynamicLaser->SetStaticMesh(cylMesh);
	}
	dynamicLaser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	dynamicLaser->SetRelativeScale3D(FVector(0.005f, 0.005f, 5.0f));
	dynamicLaser->SetRelativeLocation(FVector(250.0f, 0.0f, 0.0f));
	dynamicLaser->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	
	// Attach the raymarching proxy mesh closely to the camera so it follows the user's head
	// in room-scale VR. A 50cm box (0.5 scale) prevents near-clip culling but ensures the ray
	// origin starts right at the eyes.
	meshComponent->SetRelativeScale3D(FVector3d(0.5, 0.5, 0.5));
	//mainVolumeRender = ARadarVolumeRender::instance;
	mainVolumeRender = FindActor<ARadarVolumeRender>();
	gui = FindActor<AImGuiController>();
	if (gui) {
		gui->Destroy();
	}
	hud = FindActor<ASlateUI>();
	if (hud) {
		hud->Destroy();
	}
	if (ARadarGameStateBase* gameState = GetWorld()->GetGameState<ARadarGameStateBase>()) {

		GlobalState* globalState = &gameState->globalState;
		callbackIds.push_back(globalState->RegisterEvent("Teleport", [this](std::string stringData, void* extraData) {
			SimpleVector3<float>* vec = (SimpleVector3<float>*)extraData;
			SetActorLocation(FVector(vec->x, vec->y, vec->z));
		}));
		
		callbackIds.push_back(globalState->RegisterEvent("TeleportCamera", [this, globalState](std::string stringData, void* extraData) {
			if (globalState->globe != NULL) {
				SimpleVector3<double> targetPos = globalState->globe->GetPointScaledDegrees(globalState->teleportLatitude, globalState->teleportLongitude, globalState->teleportAltitude);
				SetActorLocation(FVector(targetPos.x, targetPos.y, targetPos.z));
			}
		}));
		
		callbackIds.push_back(globalState->RegisterEvent("GlobeUpdate", [this, globalState](std::string stringData, void* extraData) {
			if (globalState->globe != NULL) {
				SimpleVector3<double> targetPos = globalState->globe->GetPointScaled(SimpleVector3<double>(currentLatLon.x, currentLatLon.y, currentLatLon.z));
				SetActorLocation(FVector(targetPos.x, targetPos.y, targetPos.z));
			}
		}));
	}
	
	AutoLocateAndEnableRadar();
}

void ARadarViewPawn::EndPlay(const EEndPlayReason::Type endPlayReason) {
	GlobalState* globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	// unregister all events
	for(auto id : callbackIds){
		globalState->UnregisterEvent(id);
	}
	Super::EndPlay(endPlayReason);
}

// Called every frame
void ARadarViewPawn::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
	if (radarMaterialInstance == NULL) {
		if (mainVolumeRender != NULL) {
			if (mainVolumeRender->radarMaterialInstance != NULL) {
				radarMaterialInstance = mainVolumeRender->radarMaterialInstance;
				meshComponent->SetMaterial(0, radarMaterialInstance);
			}
		}
	}
	
	
	if (ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>()){
		GlobalState* globalState = &GS->globalState;
		moveSpeed = globalState->moveSpeed * (1 + speedBoost * 3);
		rotateSpeed = globalState->rotateSpeed;


	if (widgetInteraction) {
		
		ALocationMarker* currentHoveredMarker = nullptr;
		
		if (!widgetInteraction->IsOverInteractableWidget()) {
			FHitResult hitResult;
			FVector start = widgetInteraction->GetComponentLocation();
			FVector end = start + (widgetInteraction->GetForwardVector() * 100000.0f);
			FCollisionQueryParams queryParams;
			queryParams.AddIgnoredActor(this);
			
			if (GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECC_Camera, queryParams)) {
				AActor* hitActor = hitResult.GetActor();
				if (hitActor) {
					currentHoveredMarker = dynamic_cast<ALocationMarker*>(hitActor);
				}
			}
		}
		
		if (lastHoveredMarker != currentHoveredMarker) {
			if (lastHoveredMarker) lastHoveredMarker->SetHovered(false);
			if (currentHoveredMarker) currentHoveredMarker->SetHovered(true);
			lastHoveredMarker = currentHoveredMarker;
		}

		if (clickAxis > 0.5f && !bWasClicked) {
			bWasClicked = true;
			widgetInteraction->PressPointerKey(EKeys::LeftMouseButton);

			if (!widgetInteraction->IsOverInteractableWidget()) {
				if (currentHoveredMarker) {
					currentHoveredMarker->OnClick();
				}
			}

		} else if (clickAxis < 0.3f && bWasClicked) {
			bWasClicked = false;
			widgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
		}
		
		// Inspector logic
		if (globalState->bInspectorEnabled) {
			if (!inspectorMesh->IsVisible()) {
				inspectorMesh->SetVisibility(true);
				inspectorText->SetVisibility(true);
				inspectorTextShadow->SetVisibility(true);
				inspectorTooltip->SetVisibility(true);
				inspectorTooltipShadow->SetVisibility(true);
			}
			
			FVector startPos = widgetInteraction->GetComponentLocation();
			FVector forwardDir = widgetInteraction->GetForwardVector();
			
			FVector probePos = startPos + forwardDir * inspectorDistance;
			inspectorMesh->SetWorldLocation(probePos);
			
			FString infoStr = TEXT("No Data");
			
			if (ARadarVolumeRender::instance != nullptr && ARadarVolumeRender::instance->radarData != nullptr) {
				RadarData* data = ARadarVolumeRender::instance->radarData;
				if (data->buffer != nullptr) {
					FVector radarPos = ARadarVolumeRender::instance->GetActorLocation();
					FVector worldOffset = probePos - radarPos;
					
					// worldOffset is in Unreal units. 1 Unreal unit = 100 meters.
					float localRadiusUnits = FMath::Sqrt(worldOffset.X * worldOffset.X + worldOffset.Y * worldOffset.Y);
					float radiusMeters = localRadiusUnits * 100.0f;
					
					float radiusIndexFloat = (radiusMeters / data->stats.pixelSize) - data->stats.innerDistance;
					
					float thetaAngle = FMath::RadiansToDegrees(FMath::Atan2(worldOffset.Y, worldOffset.X)) + 90.0f;
					if (thetaAngle < 0.0f) thetaAngle += 360.0f;
					if (thetaAngle >= 360.0f) thetaAngle -= 360.0f;
					
					float vScale = FMath::Max(globalState->verticalScale, 0.001f);
					float trueZ = worldOffset.Z / vScale;
					float elevationAngle = FMath::RadiansToDegrees(FMath::Atan2(trueZ, localRadiusUnits));
					
					int sweepIndex = -1;
					int thetaIndex = -1;
					int radiusIndex = FMath::Clamp(FMath::RoundToInt(radiusIndexFloat), 0, data->radiusBufferCount - 1);
					
					if (radiusIndex >= 0 && radiusIndex < data->radiusBufferCount) {
						int bestSweep = -1;
						float minSweepDiff = 999.0f;
						for (int i = 0; i < data->sweepBufferCount; i++) {
							if (data->sweepInfo[i].id != -1) {
								float diff = FMath::Abs(data->sweepInfo[i].elevationAngle - elevationAngle);
								if (diff < minSweepDiff) {
									minSweepDiff = diff;
									bestSweep = i;
								}
							}
						}
						sweepIndex = bestSweep;
						
						if (sweepIndex != -1) {
							int rayInfoOffset = (data->thetaBufferCount + 2) * sweepIndex + 1;
							float minRayDiff = 999.0f;
							int bestI = -1;
							for (int i = 0; i < data->thetaBufferCount; i++) {
								RadarData::RayInfo& ray = data->rayInfo[rayInfoOffset + i];
								if (!ray.interpolated) {
									float diff = FMath::Abs(ray.actualAngle - thetaAngle);
									if (diff > 180.0f) diff = 360.0f - diff;
									if (diff < minRayDiff) {
										minRayDiff = diff;
										bestI = i;
									}
								}
							}
							
							float heightAGLMeters = trueZ * 100.0f;
							float heightAGLFeet = heightAGLMeters * 3.28084f;
							
							if (bestI != -1) {
								float maxVal = -999.0f;
								bool foundData = false;
								
								int radiusRange = 2; // search +/- 2 gates
								int thetaRange = 2;  // search +/- 2 rays
								int sweepRange = 1;  // search +/- 1 sweep to account for spatial interpolation gaps
								
								for (int s = FMath::Max(0, sweepIndex - sweepRange); s <= FMath::Min(data->sweepBufferCount - 1, sweepIndex + sweepRange); s++) {
									for (int t = bestI - thetaRange; t <= bestI + thetaRange; t++) {
										int safeT = t;
										if (safeT < 0) safeT += data->thetaBufferCount;
										if (safeT >= data->thetaBufferCount) safeT -= data->thetaBufferCount;
										
										for (int r = FMath::Max(0, radiusIndex - radiusRange); r <= FMath::Min(data->radiusBufferCount - 1, radiusIndex + radiusRange); r++) {
											int bufferIndex = s * data->sweepBufferSize + (safeT + 1) * data->thetaBufferSize + r;
											if (bufferIndex >= 0 && bufferIndex < data->fullBufferSize) {
												float val = data->buffer[bufferIndex];
												if (val != data->stats.noDataValue) {
													if (!foundData || val > maxVal) {
														maxVal = val;
														foundData = true;
													}
												}
											}
										}
									}
								}
								
								if (foundData) {
									infoStr = FString::Printf(TEXT("%.1f dBZ\n%.0f ft AGL"), maxVal, heightAGLFeet);
								} else {
									infoStr = FString::Printf(TEXT("No Data\n%.0f ft AGL"), heightAGLFeet);
								}
							} else {
								infoStr = FString::Printf(TEXT("No Data\n%.0f ft AGL"), heightAGLFeet);
							}
						}
					}
				}
			}
			
			inspectorText->SetText(FText::FromString(infoStr));
			inspectorTextShadow->SetText(FText::FromString(infoStr));
			
			// make text face camera
			FVector textPos = inspectorText->GetComponentLocation();
			FVector camPos = camera->GetComponentLocation();
			FRotator textRot = (camPos - textPos).Rotation();
			inspectorText->SetWorldRotation(textRot);
			
			float dist = FVector::Dist(camPos, textPos);
			float newScale = FMath::Max(0.1f, dist / 2000.0f);
			inspectorText->SetWorldScale3D(FVector(newScale));
			
		} else {
			if (inspectorMesh->IsVisible()) {
				inspectorMesh->SetVisibility(false);
				inspectorText->SetVisibility(false);
				inspectorTextShadow->SetVisibility(false);
				inspectorTooltip->SetVisibility(false);
				inspectorTooltipShadow->SetVisibility(false);
			}
		}

	}


		bool shouldEnableTAA = globalState->temporalAntiAliasing;
		FVector location = GetActorLocation();
		location += camera->GetForwardVector() * forwardMovement * deltaTime * moveSpeed;
		location += camera->GetRightVector() * sidewaysMovement * deltaTime * moveSpeed;
		location.Z += verticalMovement * deltaTime * moveSpeed;
		SetActorLocation(location);
		
		if(forwardMovement != 0 || sidewaysMovement != 0 || verticalMovement != 0){
			// disable TAA when moving
			shouldEnableTAA = false;
		}

		FRotator rotation = GetActorRotation();
		if(globalState->vrMode){
			rotation.Pitch = 0;
			shouldEnableTAA = false;
		}else{
			rotation.Pitch = FMath::Clamp(rotation.Pitch + (verticalRotation * deltaTime + verticalRotationAmount / 60) * rotateSpeed, -89.0f, 89.0f);
			camera->SetRelativeLocation(FVector(0, 0, 0));
			camera->SetRelativeRotation(FRotator(0, 0, 0));
		}
		verticalRotationAmount = 0;
		rotation.Yaw += (horizontalRotation * deltaTime + horizontalRotationAmount / 60) * rotateSpeed;
		horizontalRotationAmount = 0;
		rotation.Roll = 0;
		SetActorRotation(rotation);
		if (hud != NULL) {
			hud->SetCompassRotation(rotation.Yaw / 360.0f);
		}
		globalState->testFloat = deltaTime;
		
		if(shouldEnableTAA && !isTAAEnabled){
			isTAAEnabled = true;
			GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 2"));
		}
		if(!shouldEnableTAA && isTAAEnabled){
			isTAAEnabled = false;
			GEngine->Exec(GetWorld(), TEXT("r.AntiAliasingMethod 0"));
		}
		
		if(isRadarVolumeViewActive && globalState->viewMode != GlobalState::VIEW_MODE_VOLUMETRIC){
			isRadarVolumeViewActive = false;
			meshComponent->SetHiddenInGame(true);
		}
		if(!isRadarVolumeViewActive && globalState->viewMode == GlobalState::VIEW_MODE_VOLUMETRIC){
			isRadarVolumeViewActive = true;
			meshComponent->SetHiddenInGame(false);
		}
		
		FVector camPos = camera->GetComponentLocation();
		
		if(globalState->globe != NULL){
			SimpleVector3<double> actorLocTmp = SimpleVector3<double>(camPos.X, camPos.Y, camPos.Z);
			currentLatLon = SimpleVector3<float>(globalState->globe->GetLocationScaled(actorLocTmp));
		}

		if(oldCameraPosition != camPos){
			SimpleVector3<float> cameraLocation = SimpleVector3<float>(camPos.X, camPos.Y, camPos.Z);
			globalState->EmitEvent("CameraMove", "", (void*)&cameraLocation);
			oldCameraPosition = camPos;
		}
	} // closes: if (ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>())

	meshComponent->SetRelativeLocation(camera->GetRelativeLocation());
	meshComponent->SetRelativeRotation(camera->GetRelativeRotation());
}

ARadarViewPawn::~ARadarViewPawn(){
	/*if (hud != NULL) {
		delete hud;
		hud = NULL;
	}*/
}

void ARadarViewPawn::AutoLocateAndEnableRadar() {
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpRequest = FHttpModule::Get().CreateRequest();
	httpRequest->SetURL("http://ip-api.com/json/");
	httpRequest->SetVerb("GET");
	httpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess){
		if(bSuccess && Response.IsValid()){
			FString content = Response->GetContentAsString();
			TSharedPtr<FJsonObject> jsonObject;
			TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(content);
			if (FJsonSerializer::Deserialize(reader, jsonObject) && jsonObject.IsValid()) {
				if (jsonObject->HasField(TEXT("lat")) && jsonObject->HasField(TEXT("lon"))) {
					float lat = jsonObject->GetNumberField(TEXT("lat"));
					float lon = jsonObject->GetNumberField(TEXT("lon"));
					
					if (ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>()) {
						GS->globalState.teleportLatitude = lat;
						GS->globalState.teleportLongitude = lon;
						GS->globalState.EmitEvent("TeleportCamera");
						
						// Find nearest site
						float minDist = 999999.0f;
						const char* bestSite = nullptr;
						for (int i = 0; i < NexradSites::numberOfSites; i++) {
							float dLat = NexradSites::sites[i].latitude - lat;
							float dLon = NexradSites::sites[i].longitude - lon;
							float dist = dLat*dLat + dLon*dLon;
							if (dist < minDist) {
								minDist = dist;
								bestSite = NexradSites::sites[i].name;
							}
						}
						
						if (bestSite != nullptr) {
							GS->globalState.downloadSiteId = std::string(bestSite);
							GS->globalState.downloadData = true;
							GS->globalState.pollData = true;
						}
					}
				}
			}
		}
	});
	httpRequest->ProcessRequest();
}

template <class T>
T* ARadarViewPawn::FindActor() {
	TArray<AActor*> FoundActors = {};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), T::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0) {
		return (T*)FoundActors[0];
	} else {
		return NULL;
	}
}

// Called to bind functionality to input
void ARadarViewPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveFB", this, &ARadarViewPawn::MoveFB);
	PlayerInputComponent->BindAxis("MoveLR", this, &ARadarViewPawn::MoveLR);
	PlayerInputComponent->BindAxis("MoveUD", this, &ARadarViewPawn::MoveUD);
	PlayerInputComponent->BindAxis("SpeedBoost", this, &ARadarViewPawn::SpeedBoost);
	PlayerInputComponent->BindAxis("RotateLR", this, &ARadarViewPawn::RotateLR);
	PlayerInputComponent->BindAxis("RotateUD", this, &ARadarViewPawn::RotateUD);
	PlayerInputComponent->BindAxis("RotateMouseLR", this, &ARadarViewPawn::RotateMouseLR);
	PlayerInputComponent->BindAxis("RotateMouseUD", this, &ARadarViewPawn::RotateMouseUD);
	PlayerInputComponent->BindAction("MouseButton", IE_Released, this, &ARadarViewPawn::ReleaseMouse);
	PlayerInputComponent->BindAction("MouseButton", IE_Pressed, this, &ARadarViewPawn::PressMouse);
	
	PlayerInputComponent->BindAction("VRClick", IE_Pressed, this, &ARadarViewPawn::VRClickPress);
	PlayerInputComponent->BindAction("VRClick", IE_Released, this, &ARadarViewPawn::VRClickRelease);
	
	PlayerInputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &ARadarViewPawn::ToggleVRMenu);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ARadarViewPawn::ToggleVRMenu);
	PlayerInputComponent->BindAction("HoldDistanceAdjust", IE_Pressed, this, &ARadarViewPawn::ButtonXPressed);
	PlayerInputComponent->BindAction("HoldDistanceAdjust", IE_Released, this, &ARadarViewPawn::ButtonXReleased);
	PlayerInputComponent->BindAction("ToggleVRMenu", IE_Pressed, this, &ARadarViewPawn::ToggleVRMenu);

	PlayerInputComponent->BindAxis("VRGripLeft", this, &ARadarViewPawn::GripLeft);
	PlayerInputComponent->BindAxis("VRGripRight", this, &ARadarViewPawn::GripRight);
	PlayerInputComponent->BindAxis("VRClickAxis", this, &ARadarViewPawn::VRClickAxisFunc);
	PlayerInputComponent->BindAxis("VRScrollRotateX", this, &ARadarViewPawn::VRScrollRotateX);
	PlayerInputComponent->BindAxis("VRScrollRotateY", this, &ARadarViewPawn::VRScrollRotateY);
}

void ARadarViewPawn::MoveFB(float value)
{
	forwardMovement = value;
	// auto Location = GetActorLocation();
	// Location += camera->GetForwardVector() * value * MoveSpeed;
	// SetActorLocation(Location);
}

void ARadarViewPawn::MoveLR(float value)
{
	sidewaysMovement = value;
	// auto Location = GetActorLocation();
	// Location += camera->GetRightVector() * value * MoveSpeed;
	// SetActorLocation(Location);
}

void ARadarViewPawn::MoveUD(float value)
{
	verticalMovement = value;
	// auto Location = GetActorLocation();
	// Location.Z += value * MoveSpeed;
	// SetActorLocation(Location);
}


void ARadarViewPawn::SpeedBoost(float value)
{
	speedBoost = value;
}

void ARadarViewPawn::RotateUD(float value)
{
	verticalRotation = value;
	// auto Rotation = GetActorRotation();
	// Rotation.Pitch = FMath::Clamp(Rotation.Pitch + value * RotateSpeed, -89.0f, 89.0f);
	// Rotation.Roll = 0;
	// SetActorRotation(Rotation);
}

void ARadarViewPawn::RotateLR(float value)
{
	horizontalRotation = value;
	// auto Rotation = GetActorRotation();
	// Rotation.Yaw += Value * RotateSpeed;
	// SetActorRotation(Rotation);
}


void ARadarViewPawn::RotateMouseUD(float value)
{
	verticalRotationAmount += value;
}

void ARadarViewPawn::RotateMouseLR(float value)
{
	horizontalRotationAmount += value;
}

void ARadarViewPawn::ReleaseMouse() {
	// return mouse to gui
	fprintf(stderr, "Relesed mouse button\n");
	if (gui != NULL) {
		gui->UnlockMouse();
	}
}

void ARadarViewPawn::PressMouse() {
	// take mouse from gui
	fprintf(stderr, "Pressed mouse button\n");
	if (gui != NULL) {
		gui->LockMouse();
	}
}

void ARadarViewPawn::VRClickPress() {
	// Raycast logic moved to Tick() via VRClickAxis
}

void ARadarViewPawn::VRClickRelease() {
	if (widgetInteraction) {
		widgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

void ARadarViewPawn::VRClickAxisFunc(float value) {
	clickAxis = value;
}

void ARadarViewPawn::GripLeft(float value) {
	leftGripState = value;
}

void ARadarViewPawn::GripRight(float value) {
	rightGripState = value;
}

void ARadarViewPawn::VRScrollRotateX(float value) {
	if (widgetInteraction && widgetInteraction->IsOverHitTestVisibleWidget()) {
		// Do nothing or handle horizontal scroll if needed
	} else {
		horizontalRotation = value;
	}
}

void ARadarViewPawn::VRScrollRotateY(float value) {
	if (widgetInteraction && widgetInteraction->IsOverHitTestVisibleWidget()) {
		if (FMath::Abs(value) > 0.1f) {
			accumulatedScrollY += value * 0.2f;
			if (accumulatedScrollY > 1.0f) {
				widgetInteraction->ScrollWheel(1.0f);
				accumulatedScrollY -= 1.0f;
			} else if (accumulatedScrollY < -1.0f) {
				widgetInteraction->ScrollWheel(-1.0f);
				accumulatedScrollY += 1.0f;
			}
		} else {
			accumulatedScrollY = 0.0f;
		}
	} else {
		ARadarGameStateBase* GS = GetWorld()->GetGameState<ARadarGameStateBase>();
		if (GS && GS->globalState.bInspectorEnabled && FMath::Abs(value) > 0.1f) {
			if (isButtonXHeld) {
				// adjust inspector distance with lower sensitivity
				inspectorDistance = FMath::Clamp(inspectorDistance + value * 10.0f, 10.0f, 5000.0f);
			}
		}
	}
}

void ARadarViewPawn::ToggleVRMenu() {
	if (vrMenuWidget) {
		vrMenuWidget->SetVisibility(!vrMenuWidget->IsVisible());
	}
}



