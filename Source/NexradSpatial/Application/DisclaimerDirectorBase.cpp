#include "DisclaimerDirectorBase.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NexradSpacialUserSettings.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"

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

void ADisclaimerDirectorBase::CheckDisclaimerStatus()
{
	FString SlotName = TEXT("NexradSpacialUserSettings");

	// Automatically attach to the player's camera so it follows their head
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		UCameraComponent* CameraComponent = PlayerPawn->FindComponentByClass<UCameraComponent>();
		if (CameraComponent)
		{
			AttachToComponent(CameraComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			
			// The WidgetComponent already has a 150 unit offset from the constructor.
			// Set the actor's offset to 0 so they don't compound, and zero the rotation.
			SetActorRelativeLocation(FVector::ZeroVector);
			SetActorRelativeRotation(FRotator::ZeroRotator);

			// CRITICAL FIX: Scale the Widget Component down directly, so it is 1 meter wide.
			// If we scale the Actor, it shrinks the 150 unit offset to 7.5 units, which pushes the UI inside your eyeballs!
			if (DisclaimerWidgetComponent)
			{
				DisclaimerWidgetComponent->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.05f));
			}
		}
	}
	
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UNexradSpacialUserSettings* Settings = Cast<UNexradSpacialUserSettings>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (Settings && Settings->bDisclaimerAccepted)
		{
			// Bypass the UI by hiding the widget
			if (DisclaimerWidgetComponent)
			{
				DisclaimerWidgetComponent->SetVisibility(false);
				DisclaimerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			
			OnDisclaimerAlreadyAccepted();
			return;
		}
	}
	
	// Otherwise, show the UI (by leaving the WidgetComponent visible and active)
	if (DisclaimerWidgetComponent)
	{
		DisclaimerWidgetComponent->SetVisibility(true);
		DisclaimerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}
