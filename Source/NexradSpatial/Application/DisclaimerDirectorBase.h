#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"
#include "DisclaimerDirectorBase.generated.h"

UCLASS()
class NEXRADSPATIAL_API ADisclaimerDirectorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ADisclaimerDirectorBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Disclaimer")
	UWidgetComponent* DisclaimerWidgetComponent;

	UFUNCTION(BlueprintCallable, Category = "Disclaimer")
	void CheckDisclaimerStatus();

	UFUNCTION(BlueprintImplementableEvent, Category = "Disclaimer")
	void OnDisclaimerAlreadyAccepted();
};
