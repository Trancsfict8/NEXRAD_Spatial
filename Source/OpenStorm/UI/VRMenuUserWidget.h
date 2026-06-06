#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VRMenuUserWidget.generated.h"

/**
 * UMG Wrapper for the pure C++ Slate VR Menu.
 * This forces the UWidgetInteractionComponent to correctly generate HitTestGrids
 * and process virtual pointer events natively.
 */
UCLASS()
class OPENSTORM_API UVRMenuUserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
