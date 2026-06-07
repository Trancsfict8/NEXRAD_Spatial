// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "RadarVolumeRender.h"
#include "../Radar/SimpleVector3.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "RadarViewPawn.generated.h"

class ASlateUI;
class AImGuiController;
class UMotionControllerComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
class UXRDeviceVisualizationComponent;

UCLASS()
class OPENSTORM_API ARadarViewPawn : public APawn
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

	void GripLeft(float value);
	void GripRight(float value);
	
private:
	float accumulatedScrollY = 0.0f;
	float leftGripState = 0.0f;
	float rightGripState = 0.0f;



public:
	UPROPERTY(EditAnywhere)
		float moveSpeed = 300.0f;
	UPROPERTY(EditAnywhere)
		float rotateSpeed = 200.0f;
	
	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* meshComponent;
	
	UPROPERTY(EditAnywhere)
		ARadarVolumeRender* mainVolumeRender = NULL;

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
	UWidgetInteractionComponent* widgetInteraction;
};
