#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LegalDisclaimerWidgetBase.generated.h"

class UButton;
class UScrollBox;

/**
 * Base class for the Legal Disclaimer Widget.
 */
UCLASS()
class NEXRADSPATIAL_API ULegalDisclaimerWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* AcceptButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UScrollBox* DisclaimerScrollBox;

	UFUNCTION()
	void OnAcceptClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Disclaimer")
	void OnDisclaimerAccepted();

	UFUNCTION(BlueprintCallable, Category = "Disclaimer")
	void AcceptAndSave();

private:
	float CountdownTimer = 3.0f;
	bool bCanAccept = false;
};
