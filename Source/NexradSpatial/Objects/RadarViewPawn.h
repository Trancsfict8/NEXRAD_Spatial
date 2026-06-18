// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "../Radar/SimpleVector3.h"
#include <string>
#include <vector>

class ARadarVolumeRender;
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "WeatherAudioManager.h"
#include "RadarViewPawn.generated.h"

class ASlateUI;
class AImGuiController;
class UMotionControllerComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
class UXRDeviceVisualizationComponent;

UCLASS()
class NEXRADSPATIAL_API ARadarViewPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARadarViewPawn();
	~ARadarViewPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	template <class T>
	T* FindActor();
	
	float forwardMovement = 0;
	float sidewaysMovement = 0;
	float verticalMovement = 0;
	float speedBoost = 0;
	float verticalRotation = 0;
	float horizontalRotation = 0;
	float verticalRotationAmount = 0;
	float horizontalRotationAmount = 0;

	float clickAxis = 0.0f;
	bool bWasClicked = false;
	void VRClickAxisFunc(float value);

	bool enableMouseLook = false;
	
	bool isTAAEnabled = false;
	
	bool isRadarVolumeViewActive = true;
	
	FVector oldCameraPosition = FVector(0, 0, 0.0000000000000001);
	SimpleVector3<float> currentLatLon = {0,0,0};
	
	class ALocationMarker* lastHoveredMarker = nullptr;
	class AGISPolyline* lastHoveredPolyline = nullptr;
	std::string lastSiteId = "";
	
	std::vector<uint64_t> callbackIds = {};

	void MoveFB(float value);
	void MoveLR(float value);
	void MoveUD(float value);
	void SpeedBoost(float value);
	void RotateLR(float value);
	void RotateUD(float value);
	void RotateMouseLR(float value);
	void RotateMouseUD(float value);
	void ReleaseMouse();
	void PressMouse();
	void VRClickPress();
	void VRClickRelease();
	void VRScrollRotateX(float value);
	void VRScrollRotateY(float value);
	void ToggleVRMenu();
	void ToggleInspector();
	
	bool isButtonXHeld = false;
	void ButtonXPressed() { isButtonXHeld = true; }
	void ButtonXReleased() { isButtonXHeld = false; }
	
	bool bIsDrawingStroke = false;
	bool bIsErasing = false;
	bool bIsButtonBHeld = false;
	bool bHasClearedLines = false;
	float buttonBPressTime = 0.0f;
	FVector lastDrawPosition;
	void ButtonBPressed();
	void ButtonBReleased();
	void DrawLineSegment(FVector start, FVector end);
	void EraseLineSegment();
	void ClearAllLines();
	
	// Inspector Tool
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* inspectorMesh = NULL;
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* dynamicLaser = NULL;
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* drawingIndicatorMesh = NULL;
	UPROPERTY(VisibleAnywhere)
	class UTextRenderComponent* inspectorText = NULL;
	UPROPERTY(VisibleAnywhere)
	class UTextRenderComponent* inspectorTextShadow = NULL;
	UPROPERTY(VisibleAnywhere)
	class UTextRenderComponent* inspectorTooltip = NULL;
	UPROPERTY(VisibleAnywhere)
	class UTextRenderComponent* inspectorTooltipShadow = NULL;
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* inspectorHaloMesh = NULL;
	
	void BuildVRAvatar();

	void GripLeft(float value);
	void GripRight(float value);
	void TriggerLeft(float value);
	
private:
	float accumulatedScrollY = 0.0f;
	float leftGripState = 0.0f;
	float rightGripState = 0.0f;
	float leftTriggerState = 0.0f;



public:
	UPROPERTY(EditAnywhere)
		float moveSpeed = 300.0f;
	UPROPERTY(EditAnywhere)
		float rotateSpeed = 200.0f;
	
	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* meshComponent;
	
	UPROPERTY(EditAnywhere)
		ARadarVolumeRender* mainVolumeRender = NULL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
		UWeatherAudioManager* weatherAudio = NULL;

	UPROPERTY(EditAnywhere)
		AImGuiController* gui = NULL;
	
	UPROPERTY(EditAnywhere)
		UMaterialInstanceDynamic* radarMaterialInstance = NULL;
	
	UPROPERTY(EditAnywhere)
		UCameraComponent * camera = NULL;

	UPROPERTY(EditAnywhere)
		ASlateUI* hud = NULL;
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* leftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* rightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UXRDeviceVisualizationComponent* leftControllerVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UXRDeviceVisualizationComponent* rightControllerVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UWidgetComponent* vrMenuWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UWidgetComponent* warningPopupWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UWidgetInteractionComponent* widgetInteraction;
	
	// Legacy fallback variables for hardware bindings
	bool bIsInterrogatorHeld = false;
	float interrogatorHoldTimer = 0.0f;
	void InterrogatorPressed();
	void InterrogatorReleased();
	void InterrogateSpatialTriggered();

	TArray<class ALocationMarker*> spawnedMarkers;
	void RemoveLastMarker();

	void AutoLocateAndEnableRadar();
};
