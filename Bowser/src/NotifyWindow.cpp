
#include <View.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <ListView.h>
#include <ScrollView.h>
#include <ListItem.h>
#include <MessageRunner.h>

#include <stdio.h>

#include "IRCDefines.h"
#include "Prompt.h"
#include "StatusView.h"
#include "Settings.h"
#include "Bowser.h"
#include "StringManip.h"
#include "NotifyWindow.h"

NotifyWindow::NotifyWindow (
	BList *notifies,
	BRect frame,
	const char *title)

	: BWindow (
		frame,
		title,
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS
			| (bowser_app->GetWindowFollowsState()
				?  (B_NOT_ANCHORED_ON_ACTIVATE | B_NO_WORKSPACE_ACTIVATION)
				:  0))
{
	frame = Bounds();

	BMenuBar *bar (new BMenuBar (frame, "menubar"));

	BMenu *menu (new BMenu ("Notify"));

	menu->AddItem (mAdd = new BMenuItem (
		"Add" B_UTF8_ELLIPSIS,
		new BMessage (M_NOTIFY_ADD),
		'A'));
	menu->AddItem (mRemove = new BMenuItem (
		"Remove",
		new BMessage (M_NOTIFY_REMOVE),
		'R'));
	menu->AddItem (mClear = new BMenuItem (
		"Clear",
		new BMessage (M_NOTIFY_CLEAR),
		'C'));

	bar->AddItem (menu);

	AddChild (bar);

	frame.top = bar->Frame().bottom + 1;
	BView *bgView (new BView (
		frame,
		"background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW));

	bgView->SetViewColor (212, 212, 212, 255);
	AddChild (bgView);

	status = new StatusView (bgView->Bounds());
	bgView->AddChild (status);

	frame = bgView->Bounds().InsetByCopy (1, 1);

	listView = new BListView (
		BRect (
			frame.left,
			frame.top,
			frame.right - B_V_SCROLL_BAR_WIDTH,
			status->Frame().top - 2),
		"list",
		B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);

	BScrollView *scroller (new BScrollView (
		"scroller",
		listView,
		B_FOLLOW_ALL_SIDES,
		0,
		false,
		true,
		B_PLAIN_BORDER));

	bgView->AddChild (scroller);

	for (int32 i = 0; i < notifies->CountItems(); ++i)
	{
		BStringItem *item ((BStringItem *)notifies->ItemAt (i));

		listView->AddItem (item);
	}

	settings = new WindowSettings (
		this,
		title, 
		BOWSER_SETTINGS);
	settings->Restore();

	listView->MakeFocus (true);
}

NotifyWindow::~NotifyWindow (void)
{
	listView->MakeEmpty();
	delete settings;
}

bool
NotifyWindow::QuitRequested (void)
{
	settings->Save();

	BMessage msg (M_NOTIFY_SHUTDOWN);
	BString serverName (GetWord (Title(), 2));

	msg.AddString ("server", serverName.String());
	bowser_app->PostMessage (&msg);
	return true;
}

void
NotifyWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NOTIFY_COMMAND:
		{
			BListItem *item;
			bool add;

			msg->FindPointer (
				"item",
				reinterpret_cast<void **>(&item));
			msg->FindBool ("add", &add);
			
			if (add)
				listView->AddItem (item);
			else
				listView->RemoveItem (item);
			
			break;
		}

		case M_NOTIFY_ADD:
		
			if (msg->HasString ("text"))
			{
				BString serverName (GetWord (Title(), 2));
				BMessage reply;

				msg->AddString ("server", serverName.String());
				be_app_messenger.SendMessage (msg, &reply);

				if (reply.HasPointer ("item"))
				{
					BListItem *item;

					reply.FindPointer (
						"item",
						reinterpret_cast<void **>(&item));
					listView->AddItem (item);
				}
			}
			else
			{
				PromptWindow *prompt (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"Nickname:",
					"Notify",
					"",
					this,
					new BMessage (M_IGNORE_ADD),
					0));
				prompt->Show();
			}
			break;

		case M_NOTIFY_REMOVE:
		{
			BListItem *item (listView->ItemAt (listView->CurrentSelection()));
			BString serverName (GetWord (Title(), 2));

			listView->RemoveItem (item);

			msg->AddPointer ("item", item);
			msg->AddString ("server", serverName.String());
			bowser_app->PostMessage (msg);
			break;
		}

		case M_NOTIFY_CLEAR:
		{
			BString serverName (GetWord (Title(), 2));
			BMessage msg (M_NOTIFY_REMOVE);

			msg.AddString ("server", serverName.String());

			while (!listView->IsEmpty())
			{
				BListItem *item (listView->LastItem());

				listView->RemoveItem (item);
				msg.AddPointer ("item", item);
			}

			bowser_app->PostMessage (&msg);
			break;
		}

		case M_NOTIFY_END:
		{
			BListItem *item;

			msg->FindPointer (
				"item", 
				reinterpret_cast<void **>(&item));
			listView->InvalidateItem (listView->IndexOf (item));
			break;
		}

		default:
			BWindow::MessageReceived (msg);
	}
}

void
NotifyWindow::MenusBeginning (void)
{
	mRemove->SetEnabled (listView->CurrentSelection() >= 0);
	mClear->SetEnabled (listView->CountItems() > 0);
}

NotifyItem::NotifyItem (const char *name, bool on_)
	: BStringItem (name),
	  on (on_)
{
}

NotifyItem::NotifyItem (const NotifyItem &item)
	: BStringItem (item.Text()),
	  on (item.on)
{
}

NotifyItem::~NotifyItem (void)
{
}

void
NotifyItem::DrawItem (BView *owner, BRect frame, bool complete)
{
	if (on)
		owner->SetHighColor (0, 0, 0, 255);
	else
		owner->SetHighColor (100, 100, 100, 255);

	if (IsSelected())
	{
		owner->SetLowColor (152, 152, 152, 255);
		owner->SetHighColor (0, 0, 0, 255);
		owner->FillRect (frame, B_SOLID_LOW);
	}
	else if (complete)
	{
		owner->SetLowColor (owner->ViewColor());
		owner->FillRect (frame, B_SOLID_LOW);
	}

	font_height fh;
	owner->GetFontHeight (&fh);

	owner->SetDrawingMode (B_OP_OVER);
	owner->DrawString (
		Text(),
		BPoint (
			frame.left + 4,
			frame.bottom - fh.descent));

	owner->SetDrawingMode (B_OP_COPY);
	owner->SetHighColor (0, 0, 0, 255);
}


bool
NotifyItem::On (void) const
{
	return on;
}

void
NotifyItem::SetOn (bool on_)
{
	on = on_;
}

