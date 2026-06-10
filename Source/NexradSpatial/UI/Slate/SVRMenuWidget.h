#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class GlobalState;

/**
 * Full VR wrist menu with tabbed layout:
 *   - Radar: opacity, cutoff, live data toggles
 *   - GPS:   lat/lon/alt sliders + jump + save favorite
 *   - Map:   scale, real life scale, tabletop/god's view mode
 *   - MR:    mixed reality passthrough info + VR mode toggle
 */
class NEXRADSPATIAL_API SVRMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVRMenuWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class EVRMenuTab : uint8
	{
		Radar,
		GPS,
		Map,
		Settings,
		Legal,
		Controls
	};

	EVRMenuTab ActiveTab;

	/** Cached pointer – refreshed via GetGlobalState() */
	GlobalState* CachedGlobalState;

	GlobalState* GetGlobalState();

	// UI Helpers for VR-sized hit targets
	TSharedRef<SWidget> MakeLabel(const FString& Text);
	TSharedRef<SWidget> MakeSlider(const FString& Label, TFunction<float()> GetVal, TFunction<void(float)> SetVal, FString FormatStr);
	TSharedRef<SWidget> MakeCheckbox(const FString& Label, TFunction<bool()> GetVal, TFunction<void(bool)> SetVal);

	TSharedRef<SWidget> BuildRadarTab();
	TSharedRef<SWidget> BuildGPSTab();
	TSharedRef<SWidget> BuildMapTab();
	TSharedRef<SWidget> BuildSettingsTab();
	TSharedRef<SWidget> BuildLegalTab();
	TSharedRef<SWidget> BuildControlsTab();
};
