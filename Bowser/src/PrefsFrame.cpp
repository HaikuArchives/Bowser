
#include <Application.h>
#include <View.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <Box.h>
#include <String.h>

#include "IRCDefines.h"
#include "Settings.h"
#include "PrefsFrame.h"

class PrefsItem : public BStringItem
{
	BString 					boxName;
	BView						*preference;

	public:

								PrefsItem (
									const char *,
									const char *,
									BView *);
	virtual					~PrefsItem (void);

	BString					BoxName (void) const;
	BView						*Preference (void);
};

PrefsFrame::PrefsFrame (BWindow *window)
	: BWindow (
		BRect (110, 110, 200, 200),	// Going to resized
		"Preferences",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_ASYNCHRONOUS_CONTROLS),

		current (0),
		setupWindow (window),
		settings (0)
{
	BRect bounds (Bounds());

	BView *bgView (new BView (
		bounds,
		"background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW));

	bgView->SetViewColor (222, 222, 222);
	AddChild (bgView);

	list = new BListView (
		BRect (10, 10, 30, bounds.bottom - 10),
		"list",
		B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES);
	
	list->SetSelectionMessage (new BMessage (M_PREFERENCE_GROUP));

	scroller = new BScrollView (
		"scroller",
		list,
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0,
		false,
		true);
	bgView->AddChild (scroller);

	container = new BBox (
		BRect (40 + B_V_SCROLL_BAR_WIDTH, 8, bounds.right - 8, bounds.bottom - 8),
		"container");

	bgView->AddChild (container);

	settings = new WindowSettings (this, "prefs", BOWSER_SETTINGS);

	settings->Restore();

	ResizeTo (bounds.Width(), bounds.Height());
}
		
PrefsFrame::~PrefsFrame (void)
{
	while (!list->IsEmpty())
	{
		PrefsItem *item ((PrefsItem *)
			list->RemoveItem (list->CountItems() - 1));

		if (item != current && item->Preference())
			delete item->Preference();

		delete item;
	}

	delete settings;
}

bool
PrefsFrame::QuitRequested (void)
{
	settings->Save();

	setupWindow->PostMessage (M_PREFS_SHUTDOWN);
	return true;
}

void
PrefsFrame::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_PREFERENCE_GROUP:
		{
			int32 index;

			msg->FindInt32 ("index", &index);

			if (index >= 0)
			{
				container->RemoveChild (current->Preference());

				current = (PrefsItem *)list->ItemAt (index);

				current->Preference()->MoveTo (10, 20);
				container->AddChild (current->Preference());
				container->SetLabel (current->Text());
			}

			break;
		}

		default:
			BWindow::MessageReceived (msg);
			break;
	}
}

void
PrefsFrame::Show (void)
{
	if (!current)
	{
		current = (PrefsItem *)list->FirstItem();
		container->AddChild (current->Preference());
		container->SetLabel (current->Text());
		list->Select (0);
	}

	BWindow::Show();
}

void
PrefsFrame::AddPreference (
	const char *label,
	const char *boxName,
	BView *preference,
	bool attach)
{
	PrefsItem *item;

	list->AddItem (item = new PrefsItem (
		label,
		boxName,
		preference));

	// Resize list and move container to fit the label
	float width (list->StringWidth (label));

	if (scroller->Bounds().Width() < width + B_V_SCROLL_BAR_WIDTH + 15)
	{
		ResizeBy (
			width + B_V_SCROLL_BAR_WIDTH + 15
			- scroller->Bounds().Width(),
			0);

		container->MoveBy (width + B_V_SCROLL_BAR_WIDTH + 15
			- scroller->Bounds().Width(),
			0);

		scroller->ResizeTo (
			width + B_V_SCROLL_BAR_WIDTH + 15,
			scroller->Bounds().Height());
	}

	// A lot of the ResizeToPreferred methods on Be's Controls
	// do not work properly unless they are attached to a window,
	// so we attach real quick and let the view resize itself
	// Note: all your AddPreference calls should go before the
	//       the Show call!
	if (preference && attach)
	{
		preference->MoveTo (10, 20);
		container->AddChild (preference);

		// Now we resize to meet the preference's needs
		BRect bounds (preference->Bounds());

		if (container->Bounds().Width() < bounds.Width() + 20)
		{
			ResizeBy (
				bounds.Width() + 20 - container->Bounds().Width(),
				0);

			container->ResizeBy (
				bounds.Width() + 20 - container->Bounds().Width(),
				0);
		}

		if (container->Bounds().Height() < bounds.Height() + 40)
		{
			ResizeBy (
				0,
				bounds.Height() + 40 - container->Bounds().Height());

			scroller->ResizeBy (
				0,
				bounds.Height() + 40 - container->Bounds().Height());

			container->ResizeBy (
				0,
				bounds.Height() + 40 - container->Bounds().Height());
		}

		// Remove it
		container->RemoveChild (preference);
	}
}

PrefsItem::PrefsItem (
	const char *label,
	const char *boxName_,
	BView *preference_)

	: BStringItem (label),
	  boxName (boxName_),
	  preference (preference_)
{
}

PrefsItem::~PrefsItem (void)
{
}

BString
PrefsItem::BoxName (void) const
{
	return boxName;
}

BView	*
PrefsItem::Preference (void)
{
	return preference;
}

