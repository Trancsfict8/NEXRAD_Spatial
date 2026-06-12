#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DisclaimerDirectorBase.generated.h"

class UWidgetComponent;

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
	void CheckDisclaimerStatus(FName MainLevelName);
};
