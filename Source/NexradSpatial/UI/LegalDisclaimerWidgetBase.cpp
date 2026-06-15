#include "LegalDisclaimerWidgetBase.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "../Application/NexradSpacialUserSettings.h"
#include "../Application/DisclaimerDirectorBase.h"

void ULegalDisclaimerWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (AcceptButton)
	{
		AcceptButton->SetIsEnabled(false);
		AcceptButton->OnClicked.AddDynamic(this, &ULegalDisclaimerWidgetBase::OnAcceptClicked);
	}
	
	CountdownTimer = 3.0f;
	bCanAccept = false;
}

void ULegalDisclaimerWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bCanAccept)
	{
		bool bScrolledToBottom = false;
		if (DisclaimerScrollBox)
		{
			// Check if we have scrolled near the bottom
			float ScrollOffset = DisclaimerScrollBox->GetScrollOffset();
			float ScrollOffsetOfEnd = DisclaimerScrollBox->GetScrollOffsetOfEnd();
			if (ScrollOffsetOfEnd > 0 && ScrollOffset >= ScrollOffsetOfEnd - 10.0f)
			{
				bScrolledToBottom = true;
			}
		}

		CountdownTimer -= InDeltaTime;

		if (CountdownTimer <= 0.0f || bScrolledToBottom)
		{
			bCanAccept = true;
			if (AcceptButton)
			{
				AcceptButton->SetIsEnabled(true);
			}
		}
	}
}

void ULegalDisclaimerWidgetBase::OnAcceptClicked()
{
	if (bCanAccept)
	{
		AcceptAndSave();
	}
}

void ULegalDisclaimerWidgetBase::AcceptAndSave()
{
	// Save the variable to SaveGame
	FString SlotName = TEXT("NexradSpacialUserSettings");
	UNexradSpacialUserSettings* Settings = Cast<UNexradSpacialUserSettings>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	
	if (!Settings)
	{
		Settings = Cast<UNexradSpacialUserSettings>(UGameplayStatics::CreateSaveGameObject(UNexradSpacialUserSettings::StaticClass()));
	}

	if (Settings)
	{
		Settings->bDisclaimerAccepted = true;
		UGameplayStatics::SaveGameToSlot(Settings, SlotName, 0);
	}

	// Destroy the physical 3D board automatically so no Blueprint logic is needed!
	TArray<AActor*> FoundDirectors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADisclaimerDirectorBase::StaticClass(), FoundDirectors);
	for (AActor* Director : FoundDirectors)
	{
		Director->Destroy();
	}

	// Just in case destroying the actor fails, forcefully rip the UI off the screen
	RemoveFromParent();

	OnDisclaimerAccepted();
}
