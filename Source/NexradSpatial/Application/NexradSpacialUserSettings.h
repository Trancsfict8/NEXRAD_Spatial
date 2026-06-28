#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NexradSpacialUserSettings.generated.h"

/**
 * Save game class for Nexrad Spacial user settings.
 */
UCLASS()
class NEXRADSPATIAL_API UNexradSpacialUserSettings : public USaveGame
{
	GENERATED_BODY()

public:
	UNexradSpacialUserSettings();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Legal")
	bool bDisclaimerAccepted;
};
