
#include <View.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <ListItem.h>
#include <Alert.h>

#include <stdio.h>

#include "IRCDefines.h"
#include "Prompt.h"
#include "StatusView.h"
#include "Settings.h"
#include "Prompt.h"
#include "Bowser.h"
#include "StringManip.h"
#include "IgnoreWindow.h"

class IgnoreValidate : public RegExValidate
{
	public:

								IgnoreValidate (void)
									: RegExValidate ("Ignore")
								{ }

	virtual					~IgnoreValidate (void)
								{ }

	virtual bool			Validate (const char *text)
								{
									if (strchr (text, '@'))
										return RegExValidate::Validate (text);
									return true;
								}
};

IgnoreWindow::IgnoreWindow (
	BList *ignores,
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

	BMenu *menu (new BMenu ("Ignore"));

	menu->AddItem (mAdd = new BMenuItem (
		"Add" B_UTF8_ELLIPSIS,
		new BMessage (M_IGNORE_ADD),
		'A'));
	menu->AddItem (mExclude = new BMenuItem (
		"Exclude",
		new BMessage (M_IGNORE_EXCLUDE),
		'E'));
	menu->AddItem (mRemove = new BMenuItem (
		"Remove",
		new BMessage (M_IGNORE_REMOVE),
		'R'));
	menu->AddItem (mClear = new BMenuItem (
		"Clear",
		new BMessage (M_IGNORE_CLEAR),
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
	status->AddItem (new StatusItem (
		"Count:", "@@@@@"),
		true);
	bgView->AddChild (status);

	frame = bgView->Bounds().InsetByCopy (1, 1);

	listView = new BOutlineListView (
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

	for (int32 i = 0; i < ignores->CountItems(); ++i)
	{
		IgnoreItem *item ((IgnoreItem *)ignores->ItemAt (i));

		listView->AddItem (item);

		for (int32 k = 0; k < item->CountExcludes(); ++k)
			listView->AddUnder (item->ExcludeAt (k), item);
	}

	UpdateCount();

	settings = new WindowSettings (
		this,
		title, 
		BOWSER_SETTINGS);
	settings->Restore();

	listView->MakeFocus (true);
}

IgnoreWindow::~IgnoreWindow (void)
{
	delete settings;
}

bool
IgnoreWindow::QuitRequested (void)
{
	settings->Save();
	listView->DeselectAll();

	BMessage aMsg (M_IGNORE_SHUTDOWN);
	BString serverName (GetWord (Title(), 2));

	aMsg.AddString ("server", serverName.String());
	bowser_app->PostMessage (&aMsg);
	return true;
}

void
IgnoreWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_IGNORE_COMMAND:
		{
			IgnoreItem *item;

			msg->FindPointer (
				"item",
				reinterpret_cast<void **>(&item));

			listView->AddItem (item);
			UpdateCount();
			break;
		}

		case M_UNIGNORE_COMMAND:
		{
			IgnoreItem *item;

			msg->FindPointer (
				"item",
				reinterpret_cast<void **>(&item));

			for (int32 i = 0; i < item->CountExcludes(); ++i)
			{
				BListItem *child (item->ExcludeAt (i));

				listView->RemoveItem (child);
			}

			listView->RemoveItem (item);
			break;
		}

		case M_EXCLUDE_COMMAND:
		{
			BListItem *iItem, *sItem;

			msg->FindPointer (
				"parent",
				reinterpret_cast<void **>(&iItem));
			msg->FindPointer (
				"item",
				reinterpret_cast<void **>(&sItem));
			listView->AddUnder (sItem, iItem);
			break;
		}

		case M_CHANGE_NICK:
		{
			for (int32 i = 0; msg->HasPointer ("item", i); ++i)
			{
				BListItem *item;
				int32 index;

				msg->FindPointer (
					"item",
					i,
					reinterpret_cast<void **>(&item));

				if ((index = listView->IndexOf (item)) >= 0)
					listView->InvalidateItem (index);
			}
			break;
		}

		case M_IGNORE_ADD:
			
			if (msg->HasString ("text"))
			{
				const char *buffer;
				int32 i;

				msg->FindString ("text", &buffer);
				
				for (i = 0; i < listView->CountItems(); ++i)
				{
					BListItem *item (listView->ItemAt (i));

					if (!item->OutlineLevel())
					{
						IgnoreItem *iItem ((IgnoreItem *)item);

						if (strcasecmp (iItem->Text(), buffer) == 0)
							break;
					}
				}

				if (i >= listView->CountItems())
				{
					IgnoreItem *item (new IgnoreItem (buffer));
					BString serverName (GetWord (Title(), 2));

					listView->AddItem (item);

					UpdateCount();

					msg->AddPointer ("item", item);
					msg->AddString ("server", serverName.String());
					bowser_app->PostMessage (msg);
				}
			}
			else
			{
				PromptWindow *prompt (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"Nick or Address:",
					"",
					this,
					new BMessage (M_IGNORE_ADD),
					new IgnoreValidate));
				prompt->Show();
			}
			break;


		case M_IGNORE_REMOVE:
		{
			BListItem *item (listView->ItemAt (listView->CurrentSelection()));
			BMessage sMsg (*msg);
			BString serverName (GetWord (Title(), 2));

			sMsg.AddString ("server", serverName.String());

			if (item->OutlineLevel())
			{
				BListItem *parent (item);
				sMsg.AddPointer ("exclude", item);

				int32 index (listView->FullListCurrentSelection());
				while (parent->OutlineLevel())
					parent = listView->FullListItemAt (--index);
				sMsg.AddPointer ("parent", parent);
				listView->RemoveItem (item);
			}
			else
			{
				IgnoreItem *iItem ((IgnoreItem *)item);

				for (int32 i = 0; i < iItem->CountExcludes(); ++i)
				{
					BListItem *child (iItem->ExcludeAt (i));

					listView->RemoveItem (child);
				}

				listView->RemoveItem (item);
				sMsg.AddPointer ("item", item);
			}
			
			bowser_app->PostMessage (&sMsg);

			UpdateCount();
			break;
		}

		case M_IGNORE_CLEAR:
		{
			BString serverName (GetWord (Title(), 2));

			msg->AddString ("server", serverName.String());

			while (!listView->IsEmpty())
			{
				IgnoreItem *item ((IgnoreItem *)listView->FirstItem());

				for (int32 i = 0; i < item->CountExcludes(); ++i)
				{
					BStringItem *child (item->ExcludeAt (i));

					listView->RemoveItem (child);
				}

				listView->RemoveItem (item);
			}
			bowser_app->PostMessage (msg);

			status->SetItemValue (0, "0");
			break;
		}

		case M_IGNORE_EXCLUDE:

			if (msg->HasString ("text"))
			{
				IgnoreItem *item ((IgnoreItem *)listView->ItemAt (
					listView->CurrentSelection()));
				const char *nick;
				int32 i;

				msg->FindString ("text", &nick);

				for (i = 0; i < item->CountExcludes(); ++i)
				{
					BStringItem *sItem (item->ExcludeAt (i));

					if (strcasecmp (sItem->Text(), nick) == 0)
						break;
				}

				if (i >= item->CountExcludes())
				{
					BStringItem *sItem (new BStringItem (nick));
					BString serverName (GetWord (Title(), 2));

					listView->AddUnder (sItem, item);
					msg->AddPointer ("item", sItem);
					msg->AddPointer ("parent", item);
					msg->AddString ("server", serverName.String());

					bowser_app->PostMessage (msg);
				}
			}
			else
			{
				PromptWindow *prompt (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"Nick:",
					"",
					this,
					new BMessage (M_IGNORE_EXCLUDE)));
				prompt->Show();
			}
			break;

		case M_STATE_CHANGE:

			if (msg->HasBool ("windowfollows"))
			{
				bool windowfollows;
	
				msg->FindBool ("windowfollows", &windowfollows);
	
				if (windowfollows)
					SetFlags (Flags()
						| B_NOT_ANCHORED_ON_ACTIVATE
						| B_NO_WORKSPACE_ACTIVATION);
				else
					SetFlags (Flags()
						& ~(B_NOT_ANCHORED_ON_ACTIVATE
						|   B_NO_WORKSPACE_ACTIVATION));
			}
			break;

		default:
			BWindow::MessageReceived (msg);
	}
}

void
IgnoreWindow::MenusBeginning (void)
{
	int32 current (listView->CurrentSelection());
	bool exclude (false);

	mRemove->SetEnabled (current >= 0);
	mClear->SetEnabled (listView->CountItems() > 0);

	if (current >= 0)
	{
		BListItem *item (listView->ItemAt (current));

		if (item->OutlineLevel() == 0)
		{
			IgnoreItem *iItem ((IgnoreItem *)item);

			exclude = iItem->IsAddress();
		}
	}

	mExclude->SetEnabled (exclude);
}

void
IgnoreWindow::UpdateCount (void)
{
	int32 count (0);

	for (int32 i = 0; i < listView->CountItems(); ++i)
	{
		BListItem *item (listView->ItemAt (i));

		if (item->OutlineLevel() == 0)
			++count;
	}

	BString buffer;
	buffer << count;
	status->SetItemValue (0, buffer.String());
}

IgnoreItem::IgnoreItem (const char *text_)
{
	text = strcpy (new char [strlen (text_) + 1], text_);

	memset (&re, 0, sizeof (re));
	regcomp (
		&re,
		text,
		REG_EXTENDED | REG_ICASE | REG_NOSUB);
}

IgnoreItem::IgnoreItem (const IgnoreItem &item)
{
	text = strcpy (new char [strlen (item.text) + 1], item.text);

	memset (&re, 0, sizeof (re));
	regcomp (
		&re,
		text,
		REG_EXTENDED | REG_ICASE | REG_NOSUB);
}

IgnoreItem::~IgnoreItem (void)
{
	delete [] text;

	while (!excludes.IsEmpty())
	{
		BStringItem *item ((BStringItem *)excludes.RemoveItem (0L));

		delete item;
	}

	regfree (&re);
}

bool
IgnoreItem::IsAddress (void) const
{
	return strchr (text, '@') != 0;
}

bool
IgnoreItem::IsMatch (const char *nick, const char *address) const
{
	if (IsAddress()
	&&  address
	&&  regexec (&re, address, 0, 0, 0) != REG_NOMATCH)
	{
		if (nick)
			for (int32 i = 0; i < excludes.CountItems(); ++i)
			{
				BStringItem *item (ExcludeAt (i));

				if (strcasecmp (nick, item->Text()) == 0)
					return false;
			}

		return true;
	}

	else if (!IsAddress() && nick)
	{
		return strcasecmp (text, nick) == 0;
	}
	
	return false;
}

const char *
IgnoreItem::Text (void) const
{
	return text;
}

void
IgnoreItem::SetText (const char *text_)
{
	delete [] text;

	text = strcpy (new char [strlen (text_) + 1], text_);
	regfree (&re);
	memset (&re, 0, sizeof (re));
	regcomp (
		&re,
		text,
		REG_EXTENDED | REG_ICASE | REG_NOSUB);
}

bool
IgnoreItem::AddExclude (const char *nick)
{
	for (int32 i = 0; i < excludes.CountItems(); ++i)
	{
		BStringItem *item (ExcludeAt (i));

		if (strcasecmp (item->Text(), nick) == 0)
			return false;
	}

	excludes.AddItem (new BStringItem (nick));
	return true;
}

bool
IgnoreItem::AddExclude (BStringItem *item)
{
	excludes.AddItem (item);
	return true;
}

bool
IgnoreItem::RemoveExclude (const char *nick)
{
	for (int32 i = 0; i < excludes.CountItems(); ++i)
	{
		BStringItem *item (ExcludeAt (i));

		if (strcasecmp (item->Text(), nick) == 0)
		{
			excludes.RemoveItem (i);

			delete item;
			return true;
		}
	}

	return false;
}

int32
IgnoreItem::CountExcludes (void) const
{
	return excludes.CountItems();
}

BStringItem *
IgnoreItem::ExcludeAt (int32 index) const
{
	return (BStringItem *)excludes.ItemAt (index);
}

void
IgnoreItem::DrawItem (BView *owner, BRect frame, bool complete)
{
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
		text,
		BPoint (
			frame.left + 4,
			frame.bottom - fh.descent));

	owner->SetDrawingMode (B_OP_COPY);
	owner->SetHighColor (0, 0, 0, 255);
}

