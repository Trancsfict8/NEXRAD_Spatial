#include "DisclaimerDirectorBase.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NexradSpacialUserSettings.h"

ADisclaimerDirectorBase::ADisclaimerDirectorBase()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(Root);

	DisclaimerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DisclaimerWidgetComponent"));
	DisclaimerWidgetComponent->SetupAttachment(Root);
	DisclaimerWidgetComponent->SetRelativeLocation(FVector(150.0f, 0.0f, 0.0f));
	DisclaimerWidgetComponent->SetDrawSize(FVector2D(1920, 1080));
	// Setup standard VR widget settings if needed
	DisclaimerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DisclaimerWidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DisclaimerWidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ADisclaimerDirectorBase::BeginPlay()
{
	Super::BeginPlay();
}

void ADisclaimerDirectorBase::CheckDisclaimerStatus(FName MainLevelName)
{
	FString SlotName = TEXT("NexradSpacialUserSettings");
	
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UNexradSpacialUserSettings* Settings = Cast<UNexradSpacialUserSettings>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (Settings && Settings->bDisclaimerAccepted)
		{
			// Bypass the UI and open the main level
			UGameplayStatics::OpenLevel(this, MainLevelName);
			return;
		}
	}
	
	// Otherwise, show the UI (by leaving the WidgetComponent visible and active)
	if (DisclaimerWidgetComponent)
	{
		DisclaimerWidgetComponent->SetVisibility(true);
	}
}
