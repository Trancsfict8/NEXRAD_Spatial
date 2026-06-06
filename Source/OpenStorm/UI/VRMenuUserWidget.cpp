#include "VRMenuUserWidget.h"
#include "Slate/SVRMenuWidget.h"

TSharedRef<SWidget> UVRMenuUserWidget::RebuildWidget()
{
	// Mount our pure C++ Slate widget inside this UMG wrapper.
	// This acts as a bridge for the WidgetInteractionComponent hit-tester.
	return SNew(SVRMenuWidget);
}
