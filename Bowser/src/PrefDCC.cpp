
#include <CheckBox.h>
#include <String.h>

#include "PrefDCC.h"

PreferenceDCC::PreferenceDCC (void)
	: BView (
		BRect (0, 0, 270, 150),
	  "PrefDCC",
	  B_FOLLOW_LEFT | B_FOLLOW_TOP,
	  B_WILL_DRAW)
{
	BRect bounds (Bounds());

	// content would go here.
	// uncomment AddPreference ("DCC"... in SetupWindow.cpp to activate 

}



PreferenceDCC::~PreferenceDCC (void)
{
}

void
PreferenceDCC::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());


}

void
PreferenceDCC::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		default:
			BView::MessageReceived (msg);
	}
}
