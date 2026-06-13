#include "SVRMenuWidget.h"
#include "Engine/Engine.h"
#include "SlateOptMacros.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "../../Objects/RadarGameStateBase.h"
#include "../../Objects/RadarViewPawn.h"
#include "../../Application/GlobalState.h"
#include "../../Radar/Globe.h"
#include "../../Radar/NexradSites/NexradSites.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "../../Radar/Products/RadarProduct.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

static FSlateFontInfo LabelFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 14);
}

static FSlateFontInfo HeaderFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 18);
}

static FSlateFontInfo TabFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 20); // 40% bigger than default 14
}

static FSlateFontInfo CheckboxFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 18); // 25% bigger than 14
}

static FLinearColor AccentColor() { return FLinearColor(0.1f, 0.6f, 1.0f); }
static FLinearColor DimText()    { return FLinearColor(0.7f, 0.7f, 0.7f); }

void SVRMenuWidget::Construct(const FArguments& InArgs)
{
	CachedGlobalState = nullptr;
	ActiveTab = EVRMenuTab::Radar;

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.95f))
		.Padding(FMargin(0))
		[
			SNew(SVerticalBox)

			// ── Header bar ──────────────────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.15f, 1.0f))
				.Padding(FMargin(16, 10))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 10, 0)
					[
						SNew(SImage)
						.Image_Lambda([]() -> const FSlateBrush* {
							static FSlateBrush* LogoBrush = nullptr;
							if (!LogoBrush) {
								LogoBrush = new FSlateBrush();
								UTexture2D* LogoTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/NEXRADSpatial_logo.NEXRADSpatial_logo"));
								if (LogoTex) {
									LogoBrush->SetResourceObject(LogoTex);
									LogoBrush->ImageSize = FVector2D(32.f, 32.f);
								}
							}
							return LogoBrush;
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString("NexradSpatial VR"))
						.Font(HeaderFont())
						.ColorAndOpacity(AccentColor())
					]
				]
			]

			// ── Tab strip ────────────────────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::Radar; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("Radar")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::GPS; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("GPS")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::Map; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("Map")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::Settings; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("Settings")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::Legal; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("Legal")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SBox).HeightOverride(60.0f)
					[
						SNew(SButton)
						.OnClicked_Lambda([this]() { ActiveTab = EVRMenuTab::Controls; Invalidate(EInvalidateWidgetReason::Layout); return FReply::Handled(); })
						[ SNew(STextBlock).Text(FText::FromString("Controls")).Font(TabFont()).Justification(ETextJustify::Center).Margin(FMargin(0, 12)) ]
					]
				]
			]

			// ── Content area (scrollable) ────────────────────────────────
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				.AnimateWheelScrolling(true)
				.WheelScrollMultiplier(4.0f)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)

					// ── RADAR TAB ──────────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::Radar ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildRadarTab()
						]
					]

					// ── GPS TAB ────────────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::GPS ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildGPSTab()
						]
					]

					// ── MAP TAB ────────────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::Map ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildMapTab()
						]
					]

					// ── SETTINGS TAB ──────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::Settings ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildSettingsTab()
						]
					]

					// ── LEGAL TAB ─────────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::Legal ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildLegalTab()
						]
					]

					// ── CONTROLS TAB ──────────────────────────────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12, 8))
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return ActiveTab == EVRMenuTab::Controls ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							BuildControlsTab()
						]
					]
				]
			]
		]
	];
}

// ────────────────────────────────────────────────────────────────────────────
// Helper: section label
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::MakeLabel(const FString& Text)
{
	FSlateFontInfo BigFont = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.Font(BigFont)
		.ColorAndOpacity(AccentColor())
		.Margin(FMargin(0, 16, 0, 8));
}

TSharedRef<SWidget> SVRMenuWidget::MakeCheckbox(const FString& Label, TFunction<bool()> GetVal, TFunction<void(bool)> SetVal)
{
	return SNew(SBox)
		.HeightOverride(65.0f) // massive hit target
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([GetVal]() { return GetVal() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([SetVal](ECheckBoxState State) { SetVal(State == ECheckBoxState::Checked); })
			.Padding(FMargin(12)) 
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(CheckboxFont()).ColorAndOpacity(FLinearColor::White)
			]
		];
}

TSharedRef<SWidget> SVRMenuWidget::MakeSlider(const FString& Label, TFunction<float()> GetVal, TFunction<void(float)> SetVal, FString FormatStr)
{
	static FSliderStyle VRSliderStyle = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
	static bool bStyleInitialized = false;
	if (!bStyleInitialized) {
		VRSliderStyle.NormalThumbImage.ImageSize = FVector2D(64, 64);
		VRSliderStyle.HoveredThumbImage.ImageSize = FVector2D(72, 72);
		VRSliderStyle.DisabledThumbImage.ImageSize = FVector2D(64, 64);
		VRSliderStyle.BarThickness = 16.0f;
		bStyleInitialized = true;
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(STextBlock)
			.Text_Lambda([Label, GetVal, FormatStr]() {
				return FText::Format(FText::FromString(FormatStr), FText::AsNumber(GetVal(), &FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(4)));
			})
			.Font(LabelFont())
			.ColorAndOpacity(DimText())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,0,0,16))
		[
			SNew(SBox)
			.HeightOverride(80.0f) // Massive slider track height for VR clicking
			.Padding(FMargin(16, 0)) // Give it some side padding so the massive thumb doesn't clip
			[
				SNew(SSlider)
				.Style(&VRSliderStyle)
				.Value_Lambda([GetVal]() { return GetVal(); })
				.OnValueChanged_Lambda([SetVal](float v) { SetVal(v); })
				.MinValue(0.0f).MaxValue(1.0f)
			]
		];
}

// ────────────────────────────────────────────────────────────────────────────
// RADAR TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildRadarTab()
{
	TSharedRef<SScrollBox> ProductList = SNew(SScrollBox);
	for (size_t i = 0; i < RadarProduct::products.size(); i++) {
		RadarProduct* prod = RadarProduct::products[i];
		if (!prod->development) {
			FString ProdName = FString(prod->name.c_str());
			int ProdVolType = prod->volumeType;
			ProductList->AddSlot().Padding(2)
			[
				SNew(SBox).HeightOverride(40.0f)[
					SNew(SButton)
					.Text(FText::FromString(ProdName))
					.OnClicked_Lambda([this, ProdVolType]() {
						if (GlobalState* gs = GetGlobalState()) {
							gs->volumeType = ProdVolType;
							gs->EmitEvent("ChangeProduct", "", &gs->volumeType);
							gs->EmitEvent("UpdateVolumeParameters");
						}
						return FReply::Handled();
					})
				]
			];
		}
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Data Product")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(SBox).HeightOverride(150.0f)
			[
				ProductList
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Animation & Time")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text(FText::FromString("<< Step")).OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->EmitEvent("BackwardStep"); return FReply::Handled(); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text_Lambda([this]() { return GetGlobalState() && GetGlobalState()->animate ? FText::FromString("Pause") : FText::FromString("Play"); }).OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->animate = !GetGlobalState()->animate; return FReply::Handled(); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text(FText::FromString("Step >>")).OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->EmitEvent("ForwardStep"); return FReply::Handled(); })
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Animate Cutoff"), 
				[this]() { return GetGlobalState() && GetGlobalState()->animateCutoff; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->animateCutoff = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text_Lambda([this]() { return GetGlobalState() ? FText::FromString(FString::Printf(TEXT("Loop Mode: %d"), (int)GetGlobalState()->animateLoopMode)) : FText::FromString("Loop Mode"); }).OnClicked_Lambda([this]() { 
						if(GetGlobalState()) {
							GetGlobalState()->animateLoopMode = (GlobalState::LoopMode)(((int)GetGlobalState()->animateLoopMode + 1) % 6);
						}
						return FReply::Handled(); 
					})
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Data Download")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Enable Live Data"), 
				[this]() { return GetGlobalState() && GetGlobalState()->downloadData; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->downloadData = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Auto-Poll New Volumes"), 
				[this]() { return GetGlobalState() && GetGlobalState()->pollData; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->pollData = v; })
		]
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("View Mode")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text(FText::FromString("3D Volume")).OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->viewMode = GlobalState::VIEW_MODE_VOLUMETRIC; return FReply::Handled(); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(2)
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton).Text(FText::FromString("2D Slice")).OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->viewMode = GlobalState::VIEW_MODE_SLICE; return FReply::Handled(); })
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Volumetric Slice"), 
				[this]() { return GetGlobalState() && GetGlobalState()->sliceVolumetric; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->sliceVolumetric = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Slice Altitude"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->sliceAltitude / 25000.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->sliceAltitude = v * 25000.0f; }, 
				TEXT("Slice Altitude: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Slice Angle"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->sliceAngle - 0.5f) / 19.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->sliceAngle = 0.5f + (v * 19.0f); }, 
				TEXT("Slice Angle (degrees): {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Slice Rotation"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->sliceVerticalRotation / 180.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->sliceVerticalRotation = v * 180.0f; }, 
				TEXT("Slice Rotation: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Display")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Opacity"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->opacityMultiplier : 1.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->opacityMultiplier = v; }, 
				TEXT("Opacity: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Cutoff"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->cutoff : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->cutoff = v; }, 
				TEXT("Cutoff: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Height Exaggeration"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->verticalScale - 1.0f) / 3.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->verticalScale = 1.0f + (v * 3.0f); }, 
				TEXT("Height Exaggeration: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Elevation Exaggeration"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->elevationExaggeration - 1.0f) / 3.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) { GetGlobalState()->elevationExaggeration = 1.0f + (v * 3.0f); GetGlobalState()->EmitEvent("GlobeUpdate"); } }, 
				TEXT("Elevation Exaggeration: {0}"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Spatial Interpolation"), 
				[this]() { return GetGlobalState() && GetGlobalState()->spatialInterpolation; }, 
				[this](bool v) { if (GlobalState* gs = GetGlobalState()) { gs->spatialInterpolation = v; gs->EmitEvent("UpdateVolumeParameters"); } })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Temporal Interpolation"), 
				[this]() { return GetGlobalState() && GetGlobalState()->temporalInterpolation; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->temporalInterpolation = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Inspector Tool (Scroll to Move)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->bInspectorEnabled; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->bInspectorEnabled = v; })
		];
}

// ────────────────────────────────────────────────────────────────────────────
// GPS TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildGPSTab()
{
	TSharedRef<SScrollBox> SiteList = SNew(SScrollBox);
	for (int i = 0; i < NexradSites::numberOfSites; i++) {
		NexradSites::Site* site = &NexradSites::sites[i];
		FString SiteName = FString(site->name);
		SiteList->AddSlot().Padding(2)
		[
			SNew(SBox).HeightOverride(40.0f)[
				SNew(SButton)
				.Text(FText::FromString(SiteName))
				.OnClicked_Lambda([this, SiteName]() {
					if (GlobalState* gs = GetGlobalState()) {
						gs->downloadSiteId = std::string(TCHAR_TO_UTF8(*SiteName));
						gs->downloadData = true;
						gs->pollData = true;
					}
					return FReply::Handled();
				})
			]
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Jump to Coordinates")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Latitude"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->teleportLatitude + 90.0f) / 180.0f : 0.5f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->teleportLatitude = v * 180.0f - 90.0f; }, 
				TEXT("Lat: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Longitude"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->teleportLongitude + 180.0f) / 360.0f : 0.5f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->teleportLongitude = v * 360.0f - 180.0f; }, 
				TEXT("Lon: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Altitude"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->teleportAltitude / 20000.0f : 0.015f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->teleportAltitude = v * 20000.0f; }, 
				TEXT("Alt: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,8))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(FMargin(0,0,4,0))
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton)
					.Text(FText::FromString("Get Location"))
					.OnClicked_Lambda([this]() {
						if (GlobalState* gs = GetGlobalState()) {
							TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpRequest = FHttpModule::Get().CreateRequest();
							httpRequest->SetURL("http://ip-api.com/json/");
							httpRequest->SetVerb("GET");
							httpRequest->OnProcessRequestComplete().BindLambda([gs](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess){
								if(bSuccess && Response.IsValid()){
									FString content = Response->GetContentAsString();
									TSharedPtr<FJsonObject> jsonObject;
									TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(content);
									if (FJsonSerializer::Deserialize(reader, jsonObject) && jsonObject.IsValid()) {
										if (jsonObject->HasField(TEXT("lat")) && jsonObject->HasField(TEXT("lon"))) {
											gs->teleportLatitude = jsonObject->GetNumberField(TEXT("lat"));
											gs->teleportLongitude = jsonObject->GetNumberField(TEXT("lon"));
											gs->EmitEvent("TeleportCamera");
										}
									}
								}
							});
							httpRequest->ProcessRequest();
						}
						return FReply::Handled();
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(FMargin(4,0,4,0))
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton)
					.Text(FText::FromString("Jump"))
					.OnClicked_Lambda([this]() {
						if (GlobalState* gs = GetGlobalState()) {
							gs->EmitEvent("TeleportCamera");
						}
						return FReply::Handled();
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(FMargin(4,0,0,0))
			[
				SNew(SBox).HeightOverride(50.0f)[
					SNew(SButton)
					.Text(FText::FromString("Save Fav"))
					.OnClicked_Lambda([this]() {
						if (GlobalState* gs = GetGlobalState()) {
							GlobalState::Waypoint wp = {};
							wp.name = "VR Saved";
							wp.latitude = gs->teleportLatitude;
							wp.longitude = gs->teleportLongitude;
							wp.altitude = gs->teleportAltitude;
							gs->locationMarkers.push_back(wp);
							gs->EmitEvent("LocationMarkersUpdate");
						}
						return FReply::Handled();
					})
				]
			]
		]
		
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Select Radar Site")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			SNew(SBox).HeightOverride(250.0f)
			[
				SiteList
			]
		];
}

// ────────────────────────────────────────────────────────────────────────────
// MAP TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildMapTab()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Map Scale")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Show Map"), 
				[this]() { return GetGlobalState() && GetGlobalState()->enableMap; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->enableMap = v; })
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Map Appearance")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Show Map Tiles"), 
				[this]() { return GetGlobalState() && GetGlobalState()->enableMapTiles; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->enableMapTiles = v; })
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Show GIS Info"), 
				[this]() { return GetGlobalState() && GetGlobalState()->enableMapGIS; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->enableMapGIS = v; })
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Show Radar Sites"), 
				[this]() { return GetGlobalState() && GetGlobalState()->enableSiteMarkers; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->enableSiteMarkers = v; })
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,12,0,0))[ MakeLabel(TEXT("NWS Active Warnings")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Tornado Warnings (Red)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->showTornadoWarnings; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->showTornadoWarnings = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Severe Thunderstorm Warnings (Yellow)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->showSevereWarnings; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->showSevereWarnings = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Flash Flood Warnings (Green)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->showFlashFloodWarnings; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->showFlashFloodWarnings = v; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Special Marine Warnings (Blue)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->showMarineWarnings; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->showMarineWarnings = v; })
		];
}

// ────────────────────────────────────────────────────────────────────────────
// SETTINGS TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildSettingsTab()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Preferences")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Use Imperial Units (ft/mph)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->useImperialUnits; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->useImperialUnits = v; })
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Performance & Rendering")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Enable TAA (Anti-Aliasing)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->temporalAntiAliasing; }, 
				[this](bool v) { if (GetGlobalState()) GetGlobalState()->temporalAntiAliasing = v; })
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeCheckbox(TEXT("Enable Volume Fuzz (Noise)"), 
				[this]() { return GetGlobalState() && GetGlobalState()->enableFuzz; }, 
				[this](bool v) { if (GetGlobalState()) { GetGlobalState()->enableFuzz = v; GetGlobalState()->EmitEvent("UpdateVolumeParameters"); } })
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Raymarching Step Size"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->qualityCustomStepSize - 0.1f) / 19.9f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->qualityCustomStepSize = 0.1f + (v * 19.9f); }, 
				TEXT("Raymarching Quality Step Size: {0} (Lower is better quality, higher is better FPS)"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Max FPS"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->maxFPS - 20.0f) / 100.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->maxFPS = 20.0f + (v * 100.0f); }, 
				TEXT("Max FPS: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Visuals")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Height Exaggeration"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->verticalScale - 1.0f) / 3.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->verticalScale = 1.0f + (v * 3.0f); }, 
				TEXT("Height Exaggeration: {0}x"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Elevation Exaggeration"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->elevationExaggeration - 1.0f) / 3.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) { GetGlobalState()->elevationExaggeration = 1.0f + (v * 3.0f); GetGlobalState()->EmitEvent("GlobeUpdate"); } }, 
				TEXT("Elevation Exaggeration: {0}x"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Map Brightness"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->mapBrightness : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->mapBrightness = v; }, 
				TEXT("Map Brightness: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("GIS Brightness"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->mapBrightnessGIS / 1.5f) : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->mapBrightnessGIS = v * 1.5f; }, 
				TEXT("GIS Map Brightness: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Controls & UI")) ]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Drawing Tool")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Line Width"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->drawingLineWidth - 1.0f) / 49.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->drawingLineWidth = 1.0f + (v * 49.0f); }, 
				TEXT("Line Width: {0}"))
		]
		
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Color Red"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->drawingColorR / 255.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->drawingColorR = v * 255.0f; }, 
				TEXT("Color Red: {0}"))
		]
		
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Color Green"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->drawingColorG / 255.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->drawingColorG = v * 255.0f; }, 
				TEXT("Color Green: {0}"))
		]
		
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Color Blue"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->drawingColorB / 255.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->drawingColorB = v * 255.0f; }, 
				TEXT("Color Blue: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Movement Speed"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->moveSpeed - 10.0f) / 1490.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->moveSpeed = 10.0f + (v * 1490.0f); }, 
				TEXT("Camera Movement Speed: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Rotation Speed"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->rotateSpeed / 300.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->rotateSpeed = v * 300.0f; }, 
				TEXT("Camera Rotation Speed: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Timers & Data")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Time Animation Speed"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->animateSpeed - 1.0f) / 19.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->animateSpeed = 1.0f + (v * 19.0f); }, 
				TEXT("Playback Animation Speed: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Hold on Last Frame"), 
				[this]() { return GetGlobalState() ? GetGlobalState()->animateHoldEnd / 5.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->animateHoldEnd = v * 5.0f; }, 
				TEXT("Hold on Last Frame: {0}s"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Cutoff Animation Speed"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->animateCutoffSpeed - 0.1f) / 1.9f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->animateCutoffSpeed = 0.1f + (v * 1.9f); }, 
				TEXT("Cutoff Animation Speed: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Download Poll Interval"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->downloadPollInterval - 30.0f) / 270.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->downloadPollInterval = 30.0f + (v * 270.0f); }, 
				TEXT("Data Download Interval: {0} seconds"))
		]

		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("System")) ]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,4))
		[
			MakeSlider(TEXT("Audio Volume"), 
				[this]() { return GetGlobalState() ? (GetGlobalState()->audioControlMultiplier - 1.0f) / 9.0f : 0.0f; },
				[this](float v) { if (GetGlobalState()) GetGlobalState()->audioControlMultiplier = 1.0f + (v * 9.0f); }, 
				TEXT("Global Audio Volume: {0}"))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,16))
		[
			SNew(SBox).HeightOverride(60.0f) // Massive hit target for button
			[
				SNew(SButton)
				.Text(FText::FromString("RESET ALL SETTINGS TO DEFAULT"))
				.OnClicked_Lambda([this]() { if(GetGlobalState()) GetGlobalState()->EmitEvent("ResetAllSettings"); return FReply::Handled(); })
			]
		];
}

// ────────────────────────────────────────────────────────────────────────────
// LEGAL TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildLegalTab()
{
	FString LegalText = TEXT(
		"NEXRAD SPATIAL - OPEN SOURCE NOTICES\n\n"
		"Nexrad Spatial is built as a derivative work of OpenStorm Radar (originally developed by Jordan Schlick), an open-source 3D volumetric radar viewer built on Unreal Engine.\n\n"
		"In accordance with the GNU General Public License version 2 (GPLv2), the core radar data ingestion, binary decoding, and processing architecture utilized within this application are fully open-source.\n\n"
		"- Original Project Authorship: Copyright (c) Jordan Schlick & Contributors.\n"
		"- Modifications & VR Experience Layer: Copyright (c) 2026 Nexrad Spatial.\n"
		"- License Agreement: This component of the application is distributed under the terms of the GNU General Public License v2. The software is provided \"AS IS\", without warranty of any kind.\n\n"
		"SOURCE CODE AVAILABILITY:\n"
		"The complete machine-readable source code for the Nexrad Spatial development project, including all modifications made to the core radar parsing engine, is publicly hosted and freely available to the community. You can view, clone, or download the full repository at:\n"
		"[INSERT YOUR PUBLIC GITHUB REPOSITORY URL HERE]"
	);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel(TEXT("Legal & Credits")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 4))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.04f, 1.0f))
			.Padding(FMargin(12))
			[
				SNew(STextBlock)
				.Text(FText::FromString(LegalText))
				.ColorAndOpacity(DimText())
				.Font(LabelFont())
				.AutoWrapText(true)
			]
		];
}

// ────────────────────────────────────────────────────────────────────────────
// CONTROLS TAB
// ────────────────────────────────────────────────────────────────────────────
TSharedRef<SWidget> SVRMenuWidget::BuildControlsTab()
{
	FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeLabel("VR Controls") ]
		+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 4))
		[
			SNew(STextBlock).Font(BodyFont).ColorAndOpacity(FLinearColor::White)
			.Text(FText::FromString(
				"Left Controller:\n"
				"- Left Trigger: Grip/Grab Map\n"
				"- Left Grip: Grip/Grab Map\n"
				"- Left Thumbstick (Up/Down): Move Forward/Backward\n"
				"- Left Thumbstick (Left/Right): Strafe Left/Right\n"
				"- Y Button: Toggle VR Wrist Menu\n"
				"- X Button (Hold): Adjust Inspector Distance (Right Thumbstick Y-Axis)\n\n"
				"Right Controller:\n"
				"- Right Trigger: Click / UI Interact / Draw (if enabled)\n"
				"- Right Grip: Grip/Grab Map\n"
				"- Right Thumbstick (Left/Right): Rotate Camera Left/Right\n"
				"- Right Thumbstick (Press): Toggle Inspector Tool\n"
				"- A Button (Press): Remove Last Marker\n"
				"- A Button (Hold 1.0s): Spatial Interrogator (Extracts GPS + Street)\n"
				"- B Button (Press): Toggle Drawing Tool\n"
				"- B Button (Hold) + Trigger: Erase Drawn Line\n"
				"- B Button (Hold 3.0s): Clear All Lines\n"
			))
		];
}
// ────────────────────────────────────────────────────────────────────────────
// GlobalState accessor (cached per-frame via world context)
// ────────────────────────────────────────────────────────────────────────────
GlobalState* SVRMenuWidget::GetGlobalState()
{
	if (CachedGlobalState) return CachedGlobalState;
	for (const FWorldContext& ctx : GEngine->GetWorldContexts()) {
		if (ctx.World()) {
			if (ARadarGameStateBase* gs = ctx.World()->GetGameState<ARadarGameStateBase>()) {
				CachedGlobalState = &gs->globalState;
				return CachedGlobalState;
			}
		}
	}
	return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
