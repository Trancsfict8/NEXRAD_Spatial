#pragma once

#include "./Data/ShapeFile.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "GISPolyline.generated.h"

class Globe;
class GISObject;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class AGISPolyline : public AActor{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* material;
	
	UPROPERTY(VisibleAnywhere)
	UMaterialInstanceDynamic* materialInstance = NULL;
	
	UPROPERTY(EditAnywhere);
	UProceduralMeshComponent* proceduralMesh;
	
	// update position of all meshes after globe update
	void DisplayObject(GISObject* object, GISGroup* group);
	
	UPROPERTY(VisibleAnywhere)
	FString WarningText;

	UPROPERTY(VisibleAnywhere)
	bool bIsWarning = false;

	// Local-space center of the warning polygon for hit testing
	FVector WarningLocalCenter = FVector::ZeroVector;
	
	// Local-space vertices of the warning polygon for exact geometric hit testing
	TArray<FVector> WarningLocalVertices;
	
	FVector GetWarningWorldCenter() const {
		return GetActorTransform().TransformPosition(WarningLocalCenter);
	}

	void SetHovered(bool bHovered);

	// Original color of the warning for restoring after hover
	FLinearColor BaseColor = FLinearColor::White;

	
	// position object onto a globe
	void PositionObject(Globe* globe);
	
	// set brightness of material
	void SetBrightness(float brightness);
	
	AGISPolyline();
	~AGISPolyline();
	
	virtual void BeginPlay() override;
	//virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	//virtual void Tick(float DeltaTime) override;
	
};