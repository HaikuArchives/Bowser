
#define MIDDLE_FACTOR			0.50

#include <Box.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <View.h>
#include <TextControl.h>
#include <StringView.h>
#include <ScrollView.h>
#include <ListView.h>
#include <ListItem.h>
#include <Button.h>
#include <Bitmap.h>
#include <MessageFilter.h>
#include <PopUpMenu.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Alert.h>
#include <MessageRunner.h>

#include <algorithm>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Bowser.h"
#include "Prompt.h"
#include "ServerWindow.h"
#include "IRCDefines.h"
#include "PrefsFrame.h"
#include "PrefGeneral.h"
#include "PrefNotify.h"
#include "PrefFont.h"
#include "PrefColor.h"
#include "PrefEvent.h"
#include "PrefCommand.h"
#include "PrefDCC.h"
#include "PrefWindow.h"
#include "ServerOptions.h"
#include "IgnoreWindow.h"
#include "NotifyWindow.h"
#include "SetupWindow.h"

// Okay, I know.  This is utter crap, and I've entangled
// the window, the view, and filter so that you should
// be disgusted. Before you call the OOP police on me,
// most of this code was written around 3am and I was blury
// eyed.  Someday, will clean up.

class SetupView : public BView
{
	public:

	BTextControl				*iPort,
									*iName,
									*iIdent;
	BListView					*iNicks;
	BScrollView					*scroller;
	BButton						*connectButton,
									*prefsButton;
	BMenuField					*mField;
	BMenuItem					*mRemove,
									*mCopy,
									*mOptions;
	BView							*logoView;
	BBitmap						*bitmap;
	BList							*servers;
	int32							current;

	const BString				*events;

									SetupView (BRect, BList *, int32, const BString *);
	virtual						~SetupView (void);
	virtual void				AttachedToWindow (void);
	virtual void				Draw (BRect);
};

class NickItem : public BStringItem
{
	public:

	uint32						which;

									NickItem (const char *label, uint32 which_)
										: BStringItem (label),
										  which (which_)
									{ }
	virtual						~NickItem (void)
									{ }
};

class NickFilter : public BMessageFilter
{
									SetupView *view;
	public:

									NickFilter (SetupView *);
	virtual						~NickFilter (void);
	virtual filter_result	Filter (BMessage *, BHandler **);
	filter_result				HandleMouse (BMessage *);
	filter_result				HandleKeys (BMessage *);
};

ServerData::ServerData (void)
	: motd (true),
	  identd (true),
	  connect (false),
	  options (0),
	  ignoreWindow (0),
	  notifyWindow (0),
	  listWindow (0),
	  instances (0),
	  connected (0),
	  notifyRunner (0),
	  lastNotify ("")
{
}
ServerData::~ServerData (void)
{
	order.erase (order.begin(), order.end());
	
	while (!ignores.IsEmpty())
	{
		IgnoreItem *item ((IgnoreItem *)ignores.RemoveItem (
			ignores.CountItems() - 1));

		delete item;
	}

	while (!notifies.IsEmpty())
	{
		NotifyItem *item ((NotifyItem *)notifies.RemoveItem (
			notifies.CountItems() - 1));

		delete item;
	}

	if (notifyRunner) delete notifyRunner;
}


void
ServerData::Copy (const ServerData &sd)
{
	port = sd.port;
	name = sd.name;
	ident = sd.ident;
	connectCmds = sd.connectCmds;
	motd = sd.motd;
	identd = sd.identd;
	connect = sd.connect;
	order = sd.order;

	for (int32 i = 0; i < sd.ignores.CountItems(); ++i)
		ignores.AddItem (new IgnoreItem
			(*((IgnoreItem *)sd.ignores.ItemAt (i))));

	for (int32 i = 0; i < sd.notifies.CountItems(); ++i)
		notifies.AddItem (new NotifyItem
			(*((NotifyItem *)sd.notifies.ItemAt (i))));
}

BListItem *
ServerData::AddNotifyNick (const char *nick)
{
	NotifyItem *item (0);
	bool found (false);

	for (int32 i = 0; i < notifies.CountItems(); ++i)
	{
		NotifyItem *nItem ((NotifyItem *)notifies.ItemAt (i));

		if (strcasecmp (nItem->Text(), nick) == 0)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		bool empty (notifies.IsEmpty());

		notifies.AddItem (item = new NotifyItem (nick, false));

		if (empty)
		{
			BMessage msg (M_NOTIFY_START);

			msg.AddString ("server", server.String());
			msg.AddString ("nick", nick);
			bowser_app->PostMessage (&msg);
		}
	}

	return item;
}

BListItem *
ServerData::RemoveNotifyNick (const char *nick)
{
	for (int32 i = 0; i < notifies.CountItems(); ++i)
	{
		NotifyItem *item ((NotifyItem *)notifies.ItemAt (i));

		if (strcasecmp (item->Text(), nick) == 0)
		{
			notifies.RemoveItem (i);
			return item;
		}
	}
	return 0;
}

SetupWindow::SetupWindow (
	BRect frame,
	BList *servers,
	int32 current,
	const BString *events)

	: BWindow (frame,
		(BString ("Bowser: Day ") << VERSION).String(),
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS
			| B_NOT_ZOOMABLE
			| (bowser_app->GetWindowFollowsState()
				?  (B_NOT_ANCHORED_ON_ACTIVATE | B_NO_WORKSPACE_ACTIVATION)
				:  0)),

		quitCount (0),
		prefsFrame (0)
{
	bgView = new SetupView (frame.OffsetToSelf (B_ORIGIN), servers, current, events);
	AddChild (bgView);

	ResizeTo (bgView->Frame().Width(), bgView->Frame().Height());

	AddShortcut('P', B_COMMAND_KEY, new BMessage(M_PREFS_BUTTON));
	
	// Connect at startup. -jamie
	for (int32 i = 0; i < servers->CountItems(); ++i)
	{
		ServerData *serverData ((ServerData *)servers->ItemAt (i));

		if (serverData->connect)
			Connect (i);
	}
}

bool
SetupWindow::QuitRequested()
{
	// We must ask the servers to shutdown.
	// They will, but things need to happen
	// in a certain order

	// We don't wan't BApplication sending
	// off random quit requests.. we ask
	// the servers, they will ask their clients

	// Be's not providing a good way to have one
	// window own another is showing here

	// borrow the bowser_app for a sec

	bowser_app->Lock();
	quitCount = 0;
	BMessage qMsg (B_QUIT_REQUESTED);

	qMsg.AddBool ("bowser:shutdown", true);

	for (int32 i = 0; i < bowser_app->CountWindows(); ++i)
	{
		ServerWindow *server (dynamic_cast<ServerWindow *>(bowser_app->WindowAt (i)));

		if (server)
		{
			++quitCount;
			server->PostMessage (&qMsg);
		}
	}
	bowser_app->Unlock();

	if (quitCount)
		return false;

	SaveMarked();

	qMsg.AddRect ("Setup Frame", Frame());
	qMsg.AddInt32 ("Setup Current", bgView->current);
	bowser_app->PostMessage (&qMsg);

	return true;
}

void
SetupWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
			
		case M_SETUP_BUTTON: // connect to server

			if (Validate())
			{
				SaveMarked();				
				Connect (bgView->current);
			}
			break;

		case M_PREFS_BUTTON: // open prefs window

			if (prefsFrame)
			{
				if (prefsFrame->Lock())
				{
					if (prefsFrame->IsHidden())
						prefsFrame->Show();
					prefsFrame->Activate (true);
					prefsFrame->Unlock();
				}
			}
			else
			{
				prefsFrame = new PrefsFrame (this);

				prefsFrame->AddPreference ("General", "General", new PreferenceGeneral);
				prefsFrame->AddPreference ("Fonts", "Fonts", new PreferenceFont, false);
				prefsFrame->AddPreference ("Colors", "Colors", new PreferenceColor);
				prefsFrame->AddPreference ("Deskbar Icon", "Deskbar Icon", new PreferenceNotify);
				prefsFrame->AddPreference ("Event", "Event", new PreferenceEvent);
				prefsFrame->AddPreference ("Command", "Command", new PreferenceCommand);
				prefsFrame->AddPreference ("DCC", "DCC", new PreferenceDCC);
				prefsFrame->AddPreference ("Window", "Window", new PreferenceWindow);
				prefsFrame->Show();
			}
			break;

		case M_PREFS_SHUTDOWN:

			prefsFrame = 0;
			break;

		case M_SERVER_NEW:
		case M_SERVER_COPY:
		{
			BMessage *pMsg (new BMessage (M_SERVER_NAME));

			pMsg->AddBool ("copy", msg->what == M_SERVER_COPY);
			PromptWindow *serverName (new PromptWindow (
				BPoint (Frame().right - 100, Frame().top + 50),
				"Server address:",
				"New Server",
				"",
				this,
				pMsg));
			serverName->Show();
			break;
		}

		case M_SERVER_NAME:
		{
			const char *address;
			ServerData *serverData, *oldServerData;
			bool copy;

			msg->FindBool ("copy", &copy);

			if (bgView->servers->CountItems())
			{
				ServerData *serverData ((ServerData *)bgView->servers->ItemAt (bgView->current));

				if (serverData->options)
					serverData->options->Hide();
			}

			if (SaveMarked())
				bgView->mField->Menu()->FindMarked()->SetMarked (false);
			msg->FindString ("text", &address);

			serverData = new ServerData;
			serverData->server = address;
			bgView->servers->AddItem (serverData);

			msg->FindBool ("copy", &copy);
			if (copy)
			{
				oldServerData = (ServerData *)bgView->servers->ItemAt (bgView->current);
				serverData->Copy (*oldServerData);
			}

			BMenuItem *item = new BMenuItem (
				address, new BMessage (M_SERVER_SELECT));

			bgView->mField->Menu()->AddItem (
				item,
				bgView->current = bgView->mField->Menu()->CountItems() - 5);

			if (bgView->mField->Menu()->FindMarked())
				bgView->mField->Menu()->FindMarked()->SetMarked (false);

			if (!bgView->iIdent->IsEnabled())
			{
				bgView->iIdent->SetEnabled (true);
				bgView->iName->SetEnabled (true);
				bgView->iPort->SetEnabled (true);
				bgView->mRemove->SetEnabled (true);
				bgView->mCopy->SetEnabled (true);
				bgView->mOptions->SetEnabled (true);
			}
			bgView->iIdent->MakeFocus (true);

			SetCurrent();
			break;
		}

		case M_SERVER_SELECT:
		{
			ServerData *serverData ((ServerData *)bgView->servers->ItemAt (bgView->current));
			ServerOptions *options (serverData->options);

			if (SaveMarked())
				bgView->mField->Menu()->FindMarked()->SetMarked (false);
			msg->FindInt32 ("index", &bgView->current);
			SetCurrent();

			serverData = (ServerData *)bgView->servers->ItemAt (bgView->current);

			if (serverData->options)
			{
				serverData->options->Lock();
				if (options)
				{
					options->Lock();
					serverData->options->MoveTo (options->Frame().LeftTop());
					options->Unlock();
				}
				serverData->options->Show();
				serverData->options->Unlock();
			}

			if (options)
			{
				options->Lock();
				options->Hide();
				options->Unlock();
			}

			break;
		}

		case M_SERVER_REMOVE:
		{
			ServerData *serverData;
			BMenuItem *item;

			if ((serverData = (ServerData *)bgView->servers->ItemAt (bgView->current)))
			{
				item = bgView->mField->Menu()->RemoveItem (bgView->current);
				bgView->servers->RemoveItem (serverData);

				if (serverData->options)
					serverData->options->PostMessage (B_QUIT_REQUESTED);

				delete item;
				delete serverData;

				if (--bgView->current < 0)
				{
					bgView->current = 0;

					if (bgView->servers->IsEmpty())
					{
						bgView->iIdent->SetEnabled (false);
						bgView->iIdent->MakeFocus (false);
						bgView->iName->SetEnabled (false);
						bgView->iName->MakeFocus (false);
						bgView->iPort->SetEnabled (false);
						bgView->iPort->MakeFocus (false);
						bgView->mRemove->SetEnabled (false);
						bgView->mCopy->SetEnabled (false);
						bgView->mOptions->SetEnabled (false);
					}
				}

				SetCurrent();
			}
			break;
		}

		case M_SERVER_OPTIONS:

			if (bgView->servers->CountItems())
			{
				ServerData *serverData ((ServerData *)bgView->servers->ItemAt (bgView->current));

				if (serverData->options)
					serverData->options->Activate (true);
				else
				{
					BRect frame (Frame().OffsetByCopy (50, 100));
					BMessenger msgr (this);

					frame.right  = floor (frame.right * 2 / 3);
				 	serverData->options = new ServerOptions (
						frame,
						serverData,
						msgr);

					serverData->options->Show();
				}
			}
			break;

		case M_SOPTIONS_SHUTDOWN:
		{
			ServerData *serverData;

			msg->FindPointer (
				"server",
				reinterpret_cast<void **>(&serverData));
			msg->PrintToStream();
			serverData->options = 0;
			break;
		}

		case M_NICK_ADD:

			if (msg->HasString ("text"))
			{
				ServerData *serverData ((ServerData *)bgView->servers->ItemAt (bgView->current));
				const char *text;
				
				msg->FindString ("text", &text);

				// Before calling AddNickname, we must make sure our order is current
				SaveMarked();
				if (bowser_app->AddNickname (serverData, text))
				{
					BListItem *item (new NickItem (
						text,
						serverData ? serverData->order.back()
						: bowser_app->GetNicknameCount() - 1));

					bgView->iNicks->AddItem (item);
					bgView->iNicks->InvalidateItem (bgView->iNicks->CountItems() - 1);
				}
				else
				{
					BString buffer;
					BAlert *alert;

					buffer << "The nickname " << text << " already exists.";
					alert = new BAlert (
						"Nick Exists",
						buffer.String(),
						"Whoops",
						0, 0,
						B_WIDTH_FROM_WIDEST,
						B_WARNING_ALERT);
					alert->Go();
				}


			}
			else 
			{
				// We use the same message what.  The menuitems
				// don't have a text element.. PromptWindow will
				// add one.
				PromptWindow *nickname (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"Nickname:",
					"Add Nickname",
					"",
					this,
					new BMessage (M_NICK_ADD)));
				nickname->Show();
			}
			break;

		case M_NICK_REMOVE:
		{
			int32 index;

			msg->FindInt32 ("index", &index);

			if (index >= 0)
			{
				NickItem *item ((NickItem *)bgView->iNicks->ItemAt (index));
				ServerData *serverData ((ServerData *)bgView->servers->ItemAt (bgView->current));

				// Before calling RemoveNickname we must make sure our order is correct
				SaveMarked();
				if (bowser_app->RemoveNickname (serverData, item->which))
				{
					// It was actually removed.  Make adjustment to our items
					for (int32 i = 0; i < bgView->iNicks->CountItems(); ++i)
					{
						NickItem *nItem ((NickItem *)bgView->iNicks->ItemAt (i));

						if (nItem->which > item->which)
							item->which--;
					}
				}

				bgView->iNicks->RemoveItem (item);
				delete item;
			}
			break;
		}

		case M_NICK_LOAD:
		{
			ServerData *sd ((ServerData *)bgView->servers->ItemAt (bgView->current));

			SaveMarked();
			for (uint32 i = 0; i < bowser_app->GetNicknameCount(); ++i)
				if (bowser_app->AddNickname (sd, bowser_app->GetNickname (i).String()))
				{
					NickItem *item (new NickItem (bowser_app->GetNickname (i).String(), i));

					bgView->iNicks->AddItem (item);
					bgView->iNicks->InvalidateItem (bgView->iNicks->CountItems() - 1);
				}

			break;
		}

		case M_STATE_CHANGE:

			if (msg->HasBool ("nickbind"))
			{
				// Application only sends this when we need to redisplay the nick content
				ServerData *sd ((ServerData *)bgView->servers->ItemAt (bgView->current));

				for (uint32 i = bgView->iNicks->CountItems(); i < sd->order.size(); ++i)
				{
					NickItem *item (new NickItem (
						bowser_app->GetNickname (
						sd->order[i]).String(), sd->order[i]));

					bgView->iNicks->AddItem (item);
					bgView->iNicks->InvalidateItem (bgView->iNicks->CountItems() - 1);
				}
			}

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

		case M_SERVER_SHUTDOWN:

			if (quitCount)
				if (--quitCount == 0)
				{
					// Take a nap before the dirt one
					// just in case everyone hasn't
					// caught up
					snooze (20000);
					PostMessage (B_QUIT_REQUESTED);
				}
			break;

		case M_SETUP_ACTIVATE:

			if (IsHidden())
				Show();

		   	if (!IsActive())
				Activate();
		   	break;

		case M_SETUP_HIDE:

				Minimize(true);		
		   	break;

		default:
			BWindow::MessageReceived (msg);
	}
}

SetupWindow::~SetupWindow()
{
}

void
SetupWindow::Connect (int32 current)
{
	ServerData *serverData ((ServerData *)bgView->servers->ItemAt (current));
	BList *nicks (new BList);
	char **events;

	for (uint32 i = 0; i < serverData->order.size(); ++i)
	{
		BString nick (bowser_app->GetNickname (
			serverData->order[i]));

		nicks->AddItem (strcpy (new char [nick.Length() + 1], nick.String()));
	}

	events = new char * [MAX_EVENTS];
	for (int32 i = 0; i < MAX_EVENTS; ++i)
		events[i] = strcpy (new char [bgView->events[i].Length() + 1],
		bgView->events[i].String());

	if (serverData->options)
		serverData->connectCmds
			= serverData->options->connectText->Text();

	BWindow *myWindow = new ServerWindow (
		serverData->server.String(),
		nicks,
		serverData->port.String(),
		serverData->name.String(),
		serverData->ident.String(),
		const_cast<const char **>(events),
		serverData->motd,
		serverData->identd,
		serverData->connectCmds.String());

		myWindow->Show();
}

void
SetupWindow::SetCurrent (void)
{
	ServerData *serverData ((ServerData *)bgView
		->servers->ItemAt (bgView->current));

	while (bgView->iNicks->CountItems())
	{
		BListItem *item (bgView->iNicks->LastItem());

		bgView->iNicks->RemoveItem (item);
		delete item;
	}

	if (serverData)
	{
		bgView->iIdent->SetText (serverData->ident.String());
		bgView->iPort->SetText (serverData->port.String());
		bgView->iName->SetText (serverData->name.String());

		if (serverData->order.empty()
		&& !bowser_app->GetNicknameBindState())

			for (uint32 i = 0; i < bowser_app->GetNicknameCount(); ++i)
			{
				serverData->order.push_back (i);
					
				BListItem *item (new NickItem (
					bowser_app->GetNickname (i).String(), i));

				bgView->iNicks->AddItem (item);
			}

		else if (!serverData->order.empty())

			for (uint32 i = 0; i < serverData->order.size(); ++i)
			{
				BListItem *item (new NickItem (
					bowser_app->GetNickname (
					serverData->order[i]).String(), serverData->order[i]));

				bgView->iNicks->AddItem (item);
			}

		bgView->mField->MenuItem()->SetLabel (serverData->server.String());
		bgView->mField->Menu()->ItemAt (bgView->current)->SetMarked (true);
	}
	else
	{
		bgView->iIdent->SetText ("");
		bgView->iPort->SetText ("");
		bgView->iName->SetText ("");

		bgView->mField->MenuItem()->SetLabel ("");
	}
}

bool
SetupWindow::Validate (void)
{
	if (bgView->current < 0
	||  bgView->mField->Menu()->CountItems() <= 3)
	{
		BAlert *alert (new BAlert (
			"No Server",
			"You have yet to add a server.  I'm a bit confused about "
			"what I should connect to.",
			"Whoops",
			0, 0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));

		alert->Go();
		return false;
	}

	ServerData *serverData ((ServerData *)bgView->servers->ItemAt
		(bgView->current));
	serverData->ident   = bgView->iIdent->Text();
	serverData->port    = bgView->iPort->Text();
	serverData->name    = bgView->iName->Text();

	for (int32 i = 0; i < bgView->iNicks->CountItems(); ++i)
	{
		NickItem *item ((NickItem *)bgView->iNicks->ItemAt (i));

		serverData->order[i] = item->which;
	}

	if (serverData->order.empty())
	{
		BAlert *alert (new BAlert (
			"No Nickname",
			"Please specify at least one nickname.",
			"Whoops",
			0, 0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));
		alert->Go();
		bgView->iNicks->MakeFocus (true);
		return false;
	}

	// They want to be shmucks and bypass validation
	// by entering spaces in a required field --
	// let em; serves them right! (once again, a programmer
	// blaims his laziness on stupid users)
	if (serverData->ident.Length() == 0)
	{
		BAlert *alert (new BAlert (
			"No ident",
			"Please specify a name for the Ident Server.  "
			"Try \"bowser\" if your unsure about what it "
			"is you should do (in life or this situation).",
			"Whoops",
			0, 0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));
		alert->Go();
		bgView->iIdent->MakeFocus (true);
		return false;
	}

	if (serverData->name.Length() == 0)
	{
		BAlert *alert (new BAlert (
			"No Name",
			"You must specify a name.  Make it up if you like.",
			"Whoops",
			0, 0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));
		alert->Go();
		bgView->iName->MakeFocus (true);
		return false;
	}

	if (serverData->port.Length() == 0
	||  atoi (serverData->port.String()) == 0)
	{
		BAlert *alert (new BAlert (
			"No Port",
			"You must specify a port number, so I can connect "
			"to the IRC server.  Come on, you know that!\n\nTry \"6667\" if you're unsure what to do (in life or this situation)",
			"Whoops",
			0, 0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));
		alert->Go();
		bgView->iPort->MakeFocus (true);
		return false;
	}

	return true;
}

bool
SetupWindow::SaveMarked (void)
{
	ServerData *serverData;

	if (bgView->mField->Menu()->FindMarked())
	{
		serverData			  = (ServerData *)bgView->servers->ItemAt (bgView->current);
		serverData->port    = bgView->iPort->Text();
		serverData->name    = bgView->iName->Text();
		serverData->ident   = bgView->iIdent->Text();

		for (int32 i = 0; i < bgView->iNicks->CountItems(); ++i)
		{
			NickItem *item ((NickItem *)bgView->iNicks->ItemAt (i));

			serverData->order[i] = item->which;
		}

		return true;
	}

	return false;
}


SetupView::SetupView (BRect frame, BList *servers_, int32 current_, const BString *events_)
	: BView (frame, "Background", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	  bitmap (0),
	  servers (servers_),
	  current (current_),
	  events (events_)
{
	ServerData *currentData (0);
	const char *labels[] =
	{
		"Nick",
		"Ident",
		"Port",
		"Alt Nick",
		"Name",
		"Server"
	};

	const int32 lengths[] =
	{
		4, 5, 4, 8, 4, 6
	};

	float widths[6], biggest (0.0);

	//  FONT SENSITIVITY!!

	frame = Bounds();
	SetViewColor(222, 222, 222);

	bitmap = BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP, "bits");
	logoView = new BView (
		bitmap->Bounds().OffsetByCopy (0, 10),
		"Logo",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW);
	AddChild (logoView);

	// We check both, because the mField has a bold
	// font .. we want everything to line up nice
	// and tidy!
	be_plain_font->GetStringWidths (
		labels, lengths, 6, widths);

	for (int32 i = 0; i < 6; ++i)
		if (widths[i] > biggest)
			biggest = widths[i];

	be_bold_font->GetStringWidths (
		labels, lengths, 6, widths);

	for (int32 i = 0; i < 6; ++i)
		if (widths[i] > biggest)
			biggest = widths[i];

	BMenu *menu (new BMenu (""));

	for (int32 i = 0; i < servers->CountItems(); ++i)
	{
		ServerData *serverData ((ServerData *)servers->ItemAt (i));
		BMenuItem *item;

		item = new BMenuItem (
			serverData->server.String(),
			new BMessage (M_SERVER_SELECT));

		menu->AddItem (item);
		if (i == current)
		{
			currentData = serverData;
			item->SetMarked (true);
		}
	}

	menu->AddSeparatorItem();
	menu->AddItem (new BMenuItem (
		"New" B_UTF8_ELLIPSIS, new BMessage (M_SERVER_NEW)));

	menu->AddItem (mCopy = new BMenuItem (
		"Copy Current" B_UTF8_ELLIPSIS, new BMessage (M_SERVER_COPY)));

	menu->AddItem (mRemove = new BMenuItem (
		"Remove Current", new BMessage (M_SERVER_REMOVE)));

	menu->AddItem (mOptions = new BMenuItem (
		"Current's Options" B_UTF8_ELLIPSIS,
		new BMessage (M_SERVER_OPTIONS)));

	mField = new BMenuField (
		BRect (
			10,
			logoView->Frame().bottom + 10,
			ceil (frame.right * MIDDLE_FACTOR) - 5,
			logoView->Frame().bottom + 40),
		"serverField",
		"Server",
		menu);

	mField->SetFont (be_bold_font);
	mField->SetDivider (biggest + 5);

	if (currentData)
		mField->MenuItem()->SetLabel (currentData->server.String());

	AddChild (mField);

	iIdent = new BTextControl (
		BRect (
			frame.left + 10,
			mField->Frame().bottom + 3,
			ceil (frame.right * MIDDLE_FACTOR) - 5,
			mField->Frame().bottom + 30),
		"Ident",
		"Ident",
		currentData ? currentData->ident.String() : "",
		NULL);
	iIdent->SetDivider (biggest + 5);
	AddChild(iIdent);

	iName = new BTextControl (
		BRect (
			frame.left + 10,
			iIdent->Frame().bottom + 1,
			ceil (frame.right * MIDDLE_FACTOR) - 5,
			iIdent->Frame().bottom + 21),
		"Name",
		"Name",
		currentData ? currentData->name.String() : "",
		NULL);
	iName->SetDivider (biggest + 5);
	AddChild(iName);
	
	iPort = new BTextControl (
		BRect (
			frame.left + 10,
			iName->Frame().bottom + 1,
			ceil (frame.right * MIDDLE_FACTOR) - 5,
			iName->Frame().bottom + 21),
		"Port",
		"Port",
		currentData ? currentData->port.String() : "",
		NULL);
	iPort->SetDivider (biggest + 5);
	AddChild(iPort);

	BStringView *nickLabel (new BStringView (
		BRect (
			ceil (frame.right * MIDDLE_FACTOR) + 15,
			logoView->Frame().bottom + 14,
			frame.right - 10,
			logoView->Frame().bottom + 24),
		"nicklabel",
		"Nick"));
	nickLabel->SetFont (be_bold_font);
	nickLabel->ResizeToPreferred();
	AddChild (nickLabel);

	iNicks = new BListView (
		BRect (
			ceil (frame.right * MIDDLE_FACTOR) + 15,
			mField->Frame().bottom + 3,
			frame.right - 11 - B_V_SCROLL_BAR_WIDTH,
			iPort->Frame().bottom),
		"nickList");

	for (uint32 i = 0; currentData && i < currentData->order.size(); ++i)
	{
		BListItem *item;

		item = new NickItem (
			bowser_app->GetNickname (
			currentData->order[i]).String(),
			currentData->order[i]);
		iNicks->AddItem (item);
	}

	scroller = new BScrollView (
		"scroller",
		iNicks,
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0,
		false,
		true);
	AddChild (scroller);

	iNicks->AddFilter (new NickFilter (this));

	connectButton = new BButton (
		BRect (
			85,
			iPort->Frame().bottom + 20,
			185,
			iPort->Frame().bottom + 40),
		"Connect",
		"Connect",
		new BMessage(M_SETUP_BUTTON));
	AddChild (connectButton);

	prefsButton = new BButton (
		BRect (
			200,
			iPort->Frame().bottom + 20,
			300,
			iPort->Frame().bottom + 40),
		"Preferences",
		"Preferences",
		new BMessage(M_PREFS_BUTTON));
	AddChild (prefsButton);

	ResizeTo (Bounds().Width(), prefsButton->Frame().bottom + 10);
}

SetupView::~SetupView (void)
{
	delete bitmap;
}

void
SetupView::AttachedToWindow (void)
{
	iIdent->MakeFocus (true);
	connectButton->MakeDefault (true);

	if (bitmap)
	{
		logoView->MoveTo (Frame().Width() / 2 - bitmap->Bounds().Width() / 2, 10);
		logoView->SetViewBitmap (bitmap);
	}

	BView::AttachedToWindow();
}

void
SetupView::Draw (BRect)
{
	rgb_color dark  = {132, 132, 132, 255};
	rgb_color light = {255, 255, 255, 255};

	BPoint middle (
		iIdent->Frame().right + (scroller->Frame().left - iIdent->Frame().right) / 2,
		scroller->Frame().top);
	float height (scroller->Frame().Height());

	BeginLineArray (2);

	AddLine (
		middle,
		middle + BPoint (0, height),
		dark);

	AddLine (
		middle + BPoint (1, 0),
		middle + BPoint (1, height),
		light);

	EndLineArray();
}

NickFilter::NickFilter (SetupView *sv)
	: BMessageFilter (B_PROGRAMMED_DELIVERY, B_LOCAL_SOURCE),
	  view (sv)
{
}

NickFilter::~NickFilter (void)
{
}

filter_result
NickFilter::Filter (BMessage *msg, BHandler **)
{
	filter_result result (B_DISPATCH_MESSAGE);

	switch (msg->what)
	{
		case B_MOUSE_DOWN:
			result = HandleMouse (msg);
			break;

		case B_KEY_DOWN:
			result = HandleKeys (msg);
			break;
	}

	return result;
}

filter_result
NickFilter::HandleMouse (BMessage *msg)
{
	filter_result result (B_DISPATCH_MESSAGE);
	uint32 buttons, modifiers;

	msg->FindInt32 ("buttons", (int32 *)(&buttons));
	msg->FindInt32 ("modifiers", (int32 *)(&modifiers));

	if ((buttons == B_SECONDARY_MOUSE_BUTTON
	&&  (modifiers & B_SHIFT_KEY)   == 0
	&&  (modifiers & B_CONTROL_KEY) == 0
	&&  (modifiers & B_OPTION_KEY)  == 0
	&&  (modifiers & B_COMMAND_KEY) == 0)
	||  (buttons == B_PRIMARY_MOUSE_BUTTON
	&&  (modifiers & B_SHIFT_KEY)   == 0
	&&  (modifiers & B_CONTROL_KEY) != 0
	&&  (modifiers & B_OPTION_KEY)  == 0
	&&  (modifiers & B_COMMAND_KEY) == 0))
	{
		BPoint point;
		int32 index (0);

		view->iNicks->MakeFocus (true);
		msg->FindPoint ("where", &point);
		index = view->iNicks->IndexOf (point);
		if (index >= 0) view->iNicks->Select (index);

		BPopUpMenu *menu (new BPopUpMenu ("nickPop"));
		BMenuItem *item;

		menu->AddItem (item = new BMenuItem ("Add" B_UTF8_ELLIPSIS, new BMessage (M_NICK_ADD)));

		if (bowser_app->GetNicknameBindState() && view->servers->CountItems() == 0)
			item->SetEnabled (false);

		BMessage *msg (new BMessage (M_NICK_REMOVE));
		msg->AddInt32 ("index", index);
		menu->AddItem (item = new BMenuItem ("Remove", msg));
		item->SetEnabled (index >= 0);
		
		if (bowser_app->GetNicknameBindState())
		{
			menu->AddSeparatorItem();
			menu->AddItem (item = new BMenuItem ("Load", new BMessage (M_NICK_LOAD)));

			item->SetEnabled (view->servers->CountItems()
				&& (int32)bowser_app->GetNicknameCount() > view->iNicks->CountItems());
		}

		menu->SetFont (be_plain_font);
		menu->SetTargetForItems (view->Window());

		menu->Go (
			view->iNicks->ConvertToScreen (point),
			true);

		delete menu;

		result = B_SKIP_MESSAGE;
	}

	return result;
}

filter_result
NickFilter::HandleKeys (BMessage *msg)
{
	filter_result result (B_DISPATCH_MESSAGE);
	uint32 modifiers;
	const char *bytes;

	msg->FindString ("bytes", &bytes);
	msg->FindInt32 ("modifiers", (int32 *)&modifiers);

	if ((modifiers & B_CONTROL_KEY) == 0
	&&  (modifiers & B_OPTION_KEY)  == 0
	&&  (modifiers & B_COMMAND_KEY) == 0)
		switch (bytes[0])
		{
			case B_INSERT:

				if ((modifiers & B_SHIFT_KEY) == 0)
				{
					BMessage msg (M_NICK_ADD);

					view->Looper()->PostMessage (&msg);
					result = B_SKIP_MESSAGE;
				}
				break;

			case B_DELETE:

				if ((modifiers & B_SHIFT_KEY) == 0)
				{
					int32 selected;

					if ((selected = view->iNicks->CurrentSelection()) >= 0)
					{
						BMessage msg (M_NICK_REMOVE);

						msg.AddInt32 ("index", selected);
						view->Looper()->PostMessage (&msg);
						result = B_SKIP_MESSAGE;
					}
				}
				break;

			case B_UP_ARROW:

				if ((modifiers & B_SHIFT_KEY) != 0)
				{
					int32 selected (view->iNicks->CurrentSelection());

					if (selected > 0)
					{
						BListItem *item;

						item = view->iNicks->RemoveItem (selected);
						view->iNicks->AddItem (item, selected - 1);
						view->iNicks->Select (selected - 1);
						view->iNicks->ScrollToSelection();
					}

					result = B_SKIP_MESSAGE;
				}
				break;

			case B_DOWN_ARROW:
				
				if ((modifiers & B_SHIFT_KEY) != 0)
				{
					int32 selected (view->iNicks->CurrentSelection());

					if (selected >= 0
					&&  selected < view->iNicks->CountItems() - 1)
					{
						BListItem *item;

						item = view->iNicks->RemoveItem (selected);
						view->iNicks->AddItem (item, selected + 1);
						view->iNicks->Select (selected + 1);
						view->iNicks->ScrollToSelection();
					}

					result = B_SKIP_MESSAGE;
				}
				break;
		}


	return result;
}
