
class BowserApp * bowser_app;

#include <Alert.h>
#include <Font.h>
#include <Messenger.h>
#include <Autolock.h>
#include <String.h>
#include <Deskbar.h>
#include <MessageRunner.h>
#include <Resources.h>

#include <algorithm>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ServerWindow.h"
#include "Settings.h"
#include "Notify.h"
#include "SetupWindow.h"
#include "IgnoreWindow.h"
#include "NotifyWindow.h"
#include "ListWindow.h"
#include "StringManip.h"
#include "Bowser.h"
#include "DCCConnect.h"
#include "AboutWindow.h"

class AppSettings : public Settings
{
	public:

	bool							stampState,
									paranoidState,
									nicknameBindState,
									messageOpenState,
									messageFocusState,
									windowFollowsState,
									hideSetupState,
									showSetupState,
									showTopicState,
									statusTopicState,
									altwSetupState,
									altwServerState,
									masterLogState,
									dateLogsState;
	BString						alsoKnownAs,
									otherNick,
									autoNickTime;
	int32							notificationMask;

	BFont							*client_font[MAX_FONTS];
	BList							servers;
	int32							current;
	BRect							setupFrame;

	vector<BString>			nicks;
	rgb_color					colors[MAX_COLORS];
	BString						events[MAX_EVENTS];
	BString						commands[MAX_COMMANDS];

	int32							deskbarId;
	BMessenger					deskbarMsgr;

									AppSettings (void);
	virtual						~AppSettings (void);
	virtual void				SavePacked (BMessage *);
	virtual void				RestorePacked (BMessage *);
	void							AddDeskbarIcon (int32);
	void							RemoveDeskbarIcon (void);
};

#ifdef DEV_BUILD
									// Debugging tools

bool								DumpReceived (false);
bool 								DumpSent (false);
bool								ViewCodes (false);
bool								ShowId (false);
bool								NotifyStatus (false);
bool								NotifyReuse (true);


#endif

int
main()
{
	// Seed it!
	srand (time (0));

	bowser_app = new BowserApp;
	bowser_app->Run();
	delete bowser_app;

	return B_OK;
}

BowserApp::BowserApp()
	: BApplication("application/x-vnd.Ink-Bowser"),
	  aboutWin (0)
{
	settings = new AppSettings;
	dcc_sid     = create_sem (1, "dcc accept");

}

BowserApp::~BowserApp (void)
{
	delete settings;
	
	delete_sem (dcc_sid);

	#if 0
	thread_id team (Team());
	int32 cookie (0);
	thread_info info;

	BString buffer;
	int32 count (0);

	while (get_next_thread_info (team, &cookie, &info) == B_NO_ERROR)
	{
		buffer << "thread: " << info.thread;
		buffer << " name:  " << info.name;
		buffer << " state: " << (int32)info.state << "\n";
		++count;
	}

	if (buffer.Length())
	{
		BAlert *alert (new BAlert (
			"Too many threads",
			buffer.String(),
			count > 1 ? "Damn" : "Cool",
			0,
			0,
			B_WIDTH_AS_USUAL,
			count > 1 ? B_STOP_ALERT : B_INFO_ALERT));
		alert->Go();
	}

	#endif
}

bool
BowserApp::QuitRequested (void)
{
	//printf ("BowserApp::QuitRequested\n");
	BMessage *msg (CurrentMessage());

	// Presents from the setup window
	// -- the app settings are responsible
	// for saving its state 
	if (msg->HasRect ("Setup Frame")
	&&  msg->HasInt32 ("Setup Current"))
	{
		msg->FindRect ("Setup Frame", &settings->setupFrame);
		msg->FindInt32 ("Setup Current", &settings->current);

		settings->Save();

		return true;
	}

	// We haven't gotten this from setup shutdown
	// Make sure it does what it has to do first
	setupWindow->PostMessage (B_QUIT_REQUESTED);
	return false;
}

BString
BowserApp::BowserVersion (void)
{
     BResources *appResources = AppResources(); 
     size_t len;
     char *str = (char *)appResources->FindResource(B_STRING_TYPE, "VERSION", &len); 
     BString output (str);
     free (str);
     return output;
}


void
BowserApp::AboutRequested (void)
{
	if (aboutWin)
	{
		aboutWin->Activate();
	}
	else
	{
		aboutWin = new AboutWindow();
		aboutWin->Show();
	}
}

void
BowserApp::ArgvReceived (int32 ac, char **av)
{
	for (int32 i = 1; i < ac; ++i)
	{
		#ifdef DEV_BUILD

		if (strcmp (av[i], "-r") == 0)
			DumpReceived = true;

		else if (strcmp (av[i], "-s") == 0)
			DumpSent = true;

		else if (strcmp (av[i], "-v") == 0)
			ViewCodes = true;

		else if (strcmp (av[i], "-i") == 0)
			ShowId = true;

		else if (strcmp (av[i], "-N") == 0)
			NotifyStatus = true;

		else if (strcmp (av[i], "-u") == 0)
			NotifyReuse = false;

		#endif
		;
	}
}

void
BowserApp::ReadyToRun (void)
{
	settings->Restore();
	(setupWindow = new SetupWindow (
		settings->setupFrame,
		&settings->servers, 
		settings->current,
		settings->events))->Show();
}

void
BowserApp::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NEW_CLIENT:
		case M_ACTIVATION_CHANGE:
		case M_QUIT_CLIENT:
		case M_NEWS_CLIENT:
		case M_NICK_CLIENT:
		case M_ID_CHANGE:
			if (settings->deskbarMsgr.IsValid())
			{
				// just pass it along
				settings->deskbarMsgr.SendMessage (msg);

				if (msg->IsSourceWaiting())
				{
					BMessage reply (B_REPLY);

					msg->SendReply (&reply);
				}
			
			}
			break;

		case M_SERVER_STARTUP:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
			
			if ((sd = FindServerData (serverName)) != 0)
				sd->instances++;
			break;
		}

		case M_SERVER_CONNECTED:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				if (sd->connected == 0 && !sd->notifies.IsEmpty())
				{
					NotifyItem *item ((NotifyItem *)sd->notifies.FirstItem());

					sd->lastNotify = item->Text();

					BMessage send (M_SERVER_SEND);

					send.AddString ("server", sd->server.String());
					send.AddString ("data", "ISON ");
					send.AddString ("data", item->Text());
					PostMessage (&send);
				}
				sd->connected++;
			}
			
			break;
		}

		case M_SERVER_SHUTDOWN:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				sd->instances--;
				sd->connected--;

				if (sd->instances <= 0)
				{
					if (sd->notifyWindow)
						sd->notifyWindow->PostMessage (B_QUIT_REQUESTED);
					if (sd->listWindow)
						sd->listWindow->PostMessage (B_QUIT_REQUESTED);
					if (sd->ignoreWindow)
						sd->ignoreWindow->PostMessage (B_QUIT_REQUESTED);
				}
			}

			setupWindow->PostMessage (msg);
			break;
		}

		case M_SERVER_SEND:
		case M_SUBMIT:
		{
			const char *serverName;

			msg->FindString ("server", &serverName);

			for (int32 i = 0; i < CountWindows(); ++i)
			{
				ServerWindow *serverWindow (dynamic_cast<ServerWindow *>(WindowAt (i)));

				if (serverWindow && serverWindow->Id().ICompare (serverName) == 0)
				{
					serverWindow->PostMessage (msg);
					break;
				}
			}

			break;
		}
		
		case M_SLASH_RECONNECT:
		{
			printf ("M_SLASH_RECONNECT\n");
			const char *serverName;

			msg->FindString ("server", &serverName);

			for (int32 i = 0; i < CountWindows(); ++i)
			{
				ServerWindow *serverWindow (dynamic_cast<ServerWindow *>(WindowAt (i)));

				if (serverWindow && serverWindow->Id().ICompare (serverName) == 0)
				{
					printf ("found it!\n");
					serverWindow->PostMessage (M_SLASH_RECONNECT);
					break;
				}
			}

			break;
		}

		case M_SETUP_ACTIVATE:
		case M_SETUP_HIDE:
		case M_PREFS_BUTTON:

			// pass it along -- not everyone
			// has a reference to the setupwindow,
			// so they send this to us
			setupWindow->PostMessage (msg);
			break;

		case CYCLE_WINDOWS:
		{
			BLooper *looper;
			BWindow *window (0);
			bool found (false);
			int32 i;

			msg->FindPointer ("server", reinterpret_cast<void **>(&looper));

			for (i = 0; i < CountWindows(); ++i)
			{
				if (!found
				&&  dynamic_cast<ServerWindow *>(WindowAt (i))
				&&  looper == WindowAt (i))
					found = true;
				else if (found && dynamic_cast<ServerWindow *>(WindowAt (i)))
				{
					window = WindowAt (i);
					break;
				}
			}

			for (i = 0; i < CountWindows() && !window; ++i)
				if (dynamic_cast<ServerWindow *>(WindowAt (i)))
					window = WindowAt (i);

			window->Activate();
			break;
		}

		case CYCLE_BACK:
		{
			BWindow *last (0);
			BLooper *looper;

			msg->FindPointer ("server", reinterpret_cast<void **>(&looper));

			for (int32 i = 0; i < CountWindows(); ++i)
			{
				if (looper == WindowAt (i))
					break;

				if (dynamic_cast<ServerWindow *>(WindowAt (i)))
					last = WindowAt (i);
			}

			if (!last)
				for (int32 i = CountWindows() - 1; i >= 0; --i)
					if (dynamic_cast<ServerWindow *>(WindowAt (i)))
					{
						last = WindowAt (i);
						break;
					}
			last->Activate();
			break;
		}

		case M_NOTIFY_USER:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0
			&&   sd->notifyWindow)
				sd->notifyWindow->PostMessage (msg);

			break;
		}

		case M_LIST_BEGIN:
		case M_LIST_EVENT:
		case M_LIST_DONE:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0
			&&   sd->listWindow)
				sd->listWindow->PostMessage (msg);

			break;
		}

		case M_IS_IGNORED:
		{
			BMessage reply (B_REPLY);
			const char *serverName;
			ServerData *sd;
			bool ignored (false);

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *nick, *address (0);

				msg->FindString ("nick", &nick);

				if (msg->HasString ("address"))
					msg->FindString ("address", &address);

				for (int32 i = 0; i < sd->ignores.CountItems(); ++i)
				{
					IgnoreItem *item ((IgnoreItem *)sd->ignores.ItemAt (i));

					if (item->IsMatch (nick, address))
					{
						ignored = true;
						reply.AddString ("rule", item->Text());
						break;
					}
				}
			}

			reply.AddBool ("ignored", ignored);
			msg->SendReply (&reply);

			break;
		}

		case M_LIST_COMMAND:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0
			&&   sd->listWindow == 0)
			{
				BString title ("List: ");
				BRect frame;

				msg->FindRect ("frame", &frame);
				title << serverName;
				sd->listWindow = new ListWindow (
					frame.OffsetBySelf (50, 50),
					title.String());
				sd->listWindow->Show();
				sd->listWindow->PostMessage (msg);
			}

			else if (sd != 0)
			{
				sd->listWindow->Activate (true);
				sd->listWindow->PostMessage (msg);
			}

			break;
		}

		case M_LIST_SHUTDOWN:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
				sd->listWindow = 0;
			break;
		}

		case M_IGNORE_WINDOW:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0
			&&   sd->ignoreWindow == 0)
			{
				BString title ("Ignored: ");
				BRect frame;

				msg->FindRect ("frame", &frame);
				title << serverName;

				sd->ignoreWindow = new IgnoreWindow (
					&sd->ignores,
					frame.OffsetBySelf (50, 50),
					title.String());
				sd->ignoreWindow->Show();
			}
			else if (sd != 0)
				sd->ignoreWindow->Activate (true);
			break;
		}

		case M_IGNORE_COMMAND:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *cmd;
				int32 i;

				msg->FindString ("cmd", &cmd);
				for (i = 0; i < sd->ignores.CountItems(); ++i)
				{
					IgnoreItem *item ((IgnoreItem *)sd->ignores.ItemAt (i));

					if (strcasecmp (cmd, item->Text()) == 0)
						break;
				}

				if (i >= sd->ignores.CountItems())
				{
					IgnoreItem *item;

					sd->ignores.AddItem (item = new IgnoreItem (cmd));
					msg->AddPointer ("item", item);
					msg->AddString ("ignorecmd", GetCommand (CMD_IGNORE).String());

					Broadcast (msg, serverName);

					if (sd->ignoreWindow)
						sd->ignoreWindow->PostMessage (msg);
				}
			}
			break;
		}

		case M_UNIGNORE_COMMAND:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *cmd;

				msg->FindString ("cmd", &cmd);
				for (int32 i = 0; i < sd->ignores.CountItems(); ++i)
				{
					IgnoreItem *item ((IgnoreItem *)sd->ignores.ItemAt (i));

					if (strcasecmp (cmd, item->Text()) == 0)
					{
						BMessage aMsg (*msg);

						if (sd->ignoreWindow)
						{
							BMessenger msgr (sd->ignoreWindow);
							BMessage reply;

							aMsg.AddPointer ("item", item);
							msgr.SendMessage (&aMsg, &reply);
						}

						sd->ignores.RemoveItem (item);
						delete item;

						BMessage cMsg (M_IGNORE_REMOVE);
						cMsg.AddPointer ("ignores", &sd->ignores);
						cMsg.AddString ("ignorecmd", GetCommand (CMD_UNIGNORE).String());
						Broadcast (&cMsg, serverName);
					}
				}
			}
			break;
		}

		case M_EXCLUDE_COMMAND:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *second, *cmd;

				msg->FindString ("second", &second);
				msg->FindString ("cmd", &cmd);

				for (int32 i = 0; i < sd->ignores.CountItems(); ++i)
				{
					IgnoreItem *iItem ((IgnoreItem *)sd->ignores.ItemAt (i));

					if (iItem->IsAddress()
					&&  strcasecmp (iItem->Text(), second) == 0)
					{
						int32 j;

						for (j = 0; j < iItem->CountExcludes(); ++j)
						{
							BStringItem *sItem ((BStringItem *)iItem->ExcludeAt (j));

							if (strcasecmp (sItem->Text(), cmd) == 0)
								break;
						}

						if (j >= iItem->CountExcludes())
						{
							BStringItem *sItem (new BStringItem (cmd));

							iItem->AddExclude (sItem);

							if (sd->ignoreWindow)
							{
								msg->AddPointer ("parent", iItem);
								msg->AddPointer ("item", sItem);
								sd->ignoreWindow->PostMessage (msg);
							}

							BMessage msg (M_IGNORE_REMOVE);
							msg.AddPointer ("ignores", &sd->ignores);
							msg.AddString ("ignorecmd", GetCommand
								(CMD_UNIGNORE).String());
							Broadcast (&msg, serverName);
						}
						break;
					}
				}
			}

			break;
		}

		case M_IGNORE_SHUTDOWN:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
				sd->ignoreWindow = 0;

			break;
		}

		case M_IGNORE_ADD:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				IgnoreItem *item;

				msg->FindPointer (
					"item",
					reinterpret_cast<void **>(&item));
				sd->ignores.AddItem (item);

				BMessage msg (M_IGNORE_COMMAND);
				msg.AddPointer ("item", item);
				msg.AddString ("ignorecmd", GetCommand (CMD_IGNORE).String());
				Broadcast (&msg, serverName);
			}
			break;
		}

		case M_IGNORE_EXCLUDE:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				IgnoreItem *iItem;
				BStringItem *item;

				msg->FindPointer (
					"parent",
					reinterpret_cast<void **>(&iItem));
				msg->FindPointer (
					"item",
					reinterpret_cast<void **>(&item));
				iItem->AddExclude (item);

				BMessage msg (M_IGNORE_REMOVE);
				msg.AddPointer ("ignores", &sd->ignores);
				msg.AddString ("ignorecmd", GetCommand (CMD_UNIGNORE).String());
				Broadcast (&msg, serverName);
			}

			break;
		}

		case M_IGNORE_REMOVE:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				if (msg->HasPointer ("exclude"))
				{
					IgnoreItem *parent;

					msg->FindPointer (
						"parent",
						reinterpret_cast<void **>(&parent));

					for (int32 i = 0; i < sd->ignores.CountItems(); ++i)
					{
						IgnoreItem *item ((IgnoreItem *)sd->ignores.ItemAt (i));

						if (item == parent)
						{
							BStringItem *exclude;

							msg->FindPointer (
								"exclude",
								reinterpret_cast<void **>(&exclude));
							item->RemoveExclude (exclude->Text());

							BMessage msg (M_IGNORE_REMOVE);
							msg.AddPointer ("ignores", &sd->ignores);
							msg.AddString ("ignorecmd", GetCommand
								(CMD_UNIGNORE).String());
							Broadcast (&msg, serverName);
							break;
						}
					}
				}
				else
				{
					IgnoreItem *item;

					msg->FindPointer (
						"item",
						reinterpret_cast<void **>(&item));

					sd->ignores.RemoveItem (item);
					delete item;

					BMessage msg (M_IGNORE_REMOVE);
					msg.AddPointer ("ignores", &sd->ignores);
					msg.AddString ("ignorecmd", GetCommand (CMD_UNIGNORE).String());
					Broadcast (&msg, serverName);
				}
			}

			break;
		}

		case M_IGNORE_CLEAR:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				while (!sd->ignores.IsEmpty())
				{
					IgnoreItem *item ((IgnoreItem *)sd->ignores.RemoveItem (0L));

					delete item;
				}

				BMessage msg (M_IGNORE_REMOVE);
				msg.AddPointer ("ignores", &sd->ignores);
				msg.AddString ("ignorecmd", GetCommand (CMD_UNIGNORE).String());
				Broadcast (&msg, serverName);
			}

			break;
		}

		case M_NOTIFY_WINDOW:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
			
			if ((sd = FindServerData (serverName)) != 0)
			{
				if (!sd->notifyWindow)
				{
					BRect frame;

					msg->FindRect ("frame", &frame);
					BString title ("Notify: ");

					title << serverName;

					sd->notifyWindow = new NotifyWindow (
						&sd->notifies,
						frame.OffsetBySelf (50, 50),
						title.String());
					sd->notifyWindow->Show();
				}
				else
					sd->notifyWindow->Activate (true);
			}

			break;
		}
		
		case M_NOTIFY_COMMAND:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *nick;
				bool add;

				msg->FindString ("cmd", &nick);
				msg->FindBool ("add", &add);

				if (add)
				{
					BListItem *item (sd->AddNotifyNick (nick));

					if (sd->notifyWindow && item)
					{
						msg->AddPointer ("item", item);
						sd->notifyWindow->PostMessage (msg);
					}
				}
				else
				{
					BListItem *item (sd->RemoveNotifyNick (nick));
					
					if (item)
					{
						if (sd->notifyWindow)
						{
							BMessenger msgr (sd->notifyWindow);
							BMessage reply;

							msg->AddPointer ("item", item);
							msgr.SendMessage (msg, &reply);
						}
						delete item;
					}
				}

			}
			break;
		}

		case M_NOTIFY_ADD:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *nick;

				msg->FindString ("text", &nick);
				BListItem *item (sd->AddNotifyNick (nick));

				if (item)
				{
					BMessage reply (B_REPLY);

					reply.AddPointer ("item", item);
					msg->SendReply (&reply);
				}
			}
			break;
		}

		case M_NOTIFY_REMOVE:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
			if ((sd = FindServerData (serverName)) != 0)
			{
				for (int32 i = 0; msg->HasPointer ("item", i); ++i)
				{
					BStringItem *item;

					msg->FindPointer (
						"item",
						i,
						reinterpret_cast<void **>(&item));
					sd->RemoveNotifyNick (item->Text());
					delete item;
				}
			}

			break;
		}

		case M_NOTIFY_END:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);

			if ((sd = FindServerData (serverName)) != 0)
			{
				NotifyItem *item (0);
				const char *nick;
				bool hit (false);

				msg->FindString ("nick", &nick);

				for (int32 i = 0; i < sd->notifies.CountItems(); ++i)
				{
					item = (NotifyItem *)sd->notifies.ItemAt (i);

					if (sd->lastNotify.ICompare (item->Text()) == 0)
					{
						bool on (sd->lastNotify.ICompare (nick) == 0);

						if (on != item->On())
						{
							item->SetOn (on);
							if (sd->notifyWindow)
							{
								msg->AddPointer ("item", item);
								sd->notifyWindow->PostMessage (msg);
							}

							BMessage msg (M_DISPLAY), packed;
							const char *expansions[1];
							BString buffer;
							rgb_color color;

							if (on)
							{
								expansions[0] = nick;
								buffer = ExpandKeyed (
									GetEvent (E_NOTIFY_ON).String(), "N", expansions);
								color = GetColor (C_JOIN);
							}
							else
							{
								expansions[0] = sd->lastNotify.String();
								buffer = ExpandKeyed (
									GetEvent (E_NOTIFY_OFF).String(), "N", expansions);
								color = GetColor (C_QUIT);
							}

							packed.AddString ("msgz", buffer.String());
							packed.AddData (
								"color",
								B_RGB_COLOR_TYPE,
								&color,
								sizeof (color));
							packed.AddBool ("timestamp", true);
							msg.AddMessage ("packed", &packed);
							Broadcast (&msg, serverName, true);
						}

						if (++i >= sd->notifies.CountItems())
							item = ((NotifyItem *)sd->notifies.FirstItem());
						else
							item = ((NotifyItem *)sd->notifies.ItemAt (i));
						hit = true;
						break;
					}
				}

				if (!hit && !sd->notifies.IsEmpty())
					// somehow the one we were looking for is now gone
					item = (NotifyItem *)sd->notifies.FirstItem();
				
				if (item)
				{
					if (sd->notifyRunner)
					{
						delete sd->notifyRunner;
						sd->notifyRunner = 0;
					}

					msg = new BMessage (M_NOTIFY_START);
					msg->AddString ("server", sd->server.String());
					msg->AddString ("nick", item->Text());

					sd->notifyRunner = new BMessageRunner (
						be_app_messenger,
						msg,
						3000000,
						1);
				}
			}
			break;
		}

		case M_NOTIFY_START:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
			if ((sd = FindServerData (serverName)) != 0)
			{
				const char *nick;

				msg->FindString ("nick", &nick);
				sd->lastNotify = nick;

				BMessage send (M_SERVER_SEND);

				send.AddString ("server", sd->server.String());
				send.AddString ("data", "ISON ");
				send.AddString ("data", nick);
				PostMessage (&send);
			}
			break;
		}

		case M_NOTIFY_SHUTDOWN:
		{
			const char *serverName;
			ServerData *sd;

			msg->FindString ("server", &serverName);
				
			if ((sd = FindServerData (serverName)) != 0)
				sd->notifyWindow = 0;

			break;
		}
		
		case M_ABOUT_CLOSE:
		{
			aboutWin = 0;
			break;
		}

		case M_DCC_PORT:
		{
			if (msg->IsSourceWaiting())
			{
				BMessage reply (B_REPLY);
				reply.AddInt32 ("sid", dcc_sid);
				msg->SendReply (&reply);
			}
			break;
		}

		
		case M_DCC_FILE_WIN:
		{
			if (dccFileWin)
			{
				dccFileWin->PostMessage (msg);
			}
			else
			{
				DCCConnect *view;
				
				msg->FindPointer ("view", reinterpret_cast<void **>(&view));
				dccFileWin = new DCCFileWindow (view, settings->windowFollowsState);
				dccFileWin->Show();
			}

			break;
		}
		
		case M_DCC_MESSENGER:

		if (msg->IsSourceWaiting())
		{
			BMessenger msgr (dccFileWin);
			BMessage reply;

			reply.AddMessenger ("msgr", msgr);
			msg->SendReply (&reply);
		}
		break;

		case M_DCC_FILE_WIN_DONE:
		{
			dccFileWin = 0;
			break;
		}

		case M_DCC_COMPLETE:
		{
			Broadcast(msg);
			break;
		}
		

		
		default:
			BApplication::MessageReceived (msg);
	}
}

//////////////////////////////////////////////////////////////////////////////

void BowserApp::StampState(bool state)
{
	if (settings->stampState != state)
	{
		settings->stampState = state;

		BMessage msg (M_STATE_CHANGE);

		msg.AddBool ("timestamp", state);
		Broadcast (&msg);
	}
}

bool
BowserApp::GetStampState (void) const
{
	return settings->stampState;
}

void
BowserApp::ParanoidState (bool state)
{
	if (settings->paranoidState != state)
	{
		settings->paranoidState = state;

		BMessage msg (M_STATE_CHANGE);

		msg.AddBool ("paranoid", state);
		Broadcast (&msg);
	}
}

bool
BowserApp::GetParanoidState (void) const
{
	return settings->paranoidState;
}

void
BowserApp::AlsoKnownAs (const char *aka)
{
	if (settings->alsoKnownAs.ICompare (aka))
	{
		settings->alsoKnownAs = aka;

		BMessage msg (M_STATE_CHANGE);

		msg.AddString ("aka", aka);
		Broadcast (&msg);
	}
}

BString
BowserApp::GetAlsoKnownAs (void) const
{
	return settings->alsoKnownAs;
}

void
BowserApp::OtherNick (const char *oNick)
{
	if (settings->otherNick.ICompare (oNick))
	{
		settings->otherNick = oNick;

		BMessage msg (M_STATE_CHANGE);

		msg.AddString ("other nick", oNick);
		Broadcast (&msg);
	}
}

BString
BowserApp::GetOtherNick (void) const
{
	return settings->otherNick;
}

void
BowserApp::AutoNickTime (const char *aNick)
{
	if (settings->autoNickTime.ICompare (aNick))
	{
		settings->autoNickTime = aNick;

		BMessage msg (M_STATE_CHANGE);

		msg.AddString ("auto nick", aNick);
		Broadcast (&msg);
	}
}

BString
BowserApp::GetAutoNickTime (void) const
{
	return settings->autoNickTime;
}

// Font preferences
void
BowserApp::ClientFontFamilyAndStyle (
	int32 which,
	const char *family,
	const char *style)
{
	if (which < MAX_FONTS && which >= 0)
	{
		settings->client_font[which]->SetFamilyAndStyle (family, style);

		BMessage msg (M_STATE_CHANGE);

		msg.AddInt32 ("which", which);
		msg.AddPointer ("font", settings->client_font[which]);
		Broadcast (&msg);
	}
}

void
BowserApp::ClientFontSize (int32 which, float size)
{
	if (which < MAX_FONTS && which >= 0)
	{
		settings->client_font[which]->SetSize (size);

		BMessage msg (M_STATE_CHANGE);

		msg.AddInt32 ("which", which);
		msg.AddPointer ("font", settings->client_font[which]);
		Broadcast (&msg);
	}
}

const BFont *
BowserApp::GetClientFont (int32 which) const
{
	return which < MAX_FONTS && which >= 0 
		? settings->client_font[which] : 0;
}

// Notification
void
BowserApp::NotificationMask (int32 mask)
{
	if (((settings->notificationMask & NOTIFY_CONT_BIT) != 0
	||   (settings->notificationMask & NOTIFY_NICK_BIT) != 0)
	&&   (mask & NOTIFY_CONT_BIT) == 0
	&&   (mask & NOTIFY_NICK_BIT) == 0)
	{
		// Remove the thing
		settings->RemoveDeskbarIcon();
	}

	else if ((settings->notificationMask & NOTIFY_CONT_BIT) == 0
	&&       (settings->notificationMask & NOTIFY_NICK_BIT) == 0
	&&      ((mask & NOTIFY_CONT_BIT) != 0
	||       (mask & NOTIFY_NICK_BIT) != 0))
	{
		// Add it
		settings->AddDeskbarIcon (mask);
	}
	
	settings->notificationMask = mask;

	// Tell everyone
	BMessage msg (M_STATE_CHANGE);
	msg.AddBool ("cannotify", settings->deskbarMsgr.IsValid());
	msg.AddInt32 ("notifymask", settings->notificationMask);
	Broadcast (&msg);
}

int32
BowserApp::GetNotificationMask (void) const
{
	return settings->notificationMask;
}

void
BowserApp::MessageOpenState (bool state)
{
	settings->messageOpenState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("messageopen", state);
	Broadcast (&msg);
}

void
BowserApp::MessageFocusState (bool state)
{
	BAutolock lock (this);

	if (lock.IsLocked())
		settings->messageFocusState = state;
}

bool
BowserApp::GetMessageFocusState (void)
{
	BAutolock lock (this);
	bool state (true);

	if (lock.IsLocked())
		state = settings->messageFocusState;

	return state;
}

void
BowserApp::WindowFollowsState (bool state)
{
	settings->windowFollowsState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("windowfollows", state);
	Broadcast (&msg);
}

bool
BowserApp::GetWindowFollowsState (void) const
{
	return settings->windowFollowsState;
}

void
BowserApp::HideSetupState (bool state)
{
	settings->hideSetupState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("hidesetup", state);
	Broadcast (&msg);
}

bool
BowserApp::GetHideSetupState (void) const
{
	return settings->hideSetupState;
}

void
BowserApp::ShowSetupState (bool state)
{
	settings->showSetupState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("showsetup", state);
	Broadcast (&msg);
}

bool
BowserApp::GetShowSetupState (void) const
{
	return settings->showSetupState;
}

void
BowserApp::ShowTopicState (bool state)
{
	settings->showTopicState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("showtopic", state);
	Broadcast (&msg);
}

bool
BowserApp::GetShowTopicState (void) const
{
	return settings->showTopicState;
}

void
BowserApp::StatusTopicState (bool state)
{
	settings->statusTopicState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("statustopic", state);
	Broadcast (&msg);
}

bool
BowserApp::GetStatusTopicState (void) const
{
	return settings->statusTopicState;
}

void
BowserApp::AltwSetupState (bool state)
{
	settings->altwSetupState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("AltW Setup", state);
	Broadcast (&msg);
}

bool
BowserApp::GetAltwSetupState (void) const
{
	return settings->altwSetupState;
}

void
BowserApp::AltwServerState (bool state)
{
	settings->altwServerState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("AltW Server", state);
	Broadcast (&msg);
}

bool
BowserApp::GetAltwServerState (void) const
{
	return settings->altwServerState;
}

void
BowserApp::MasterLogState (bool state)
{
	settings->masterLogState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("Master Log", state);
	Broadcast (&msg);
}

bool
BowserApp::GetMasterLogState (void) const
{
	return settings->masterLogState;
}

void
BowserApp::DateLogsState (bool state)
{
	settings->dateLogsState = state;

	BMessage msg (M_STATE_CHANGE);

	msg.AddBool ("Date Logs", state);
	Broadcast (&msg);
}

bool
BowserApp::GetDateLogsState (void) const
{
	return settings->dateLogsState;
}

bool
BowserApp::GetMessageOpenState (void) const
{
	return settings->messageOpenState;
}

rgb_color
BowserApp::GetColor (int32 which) const
{
	rgb_color color = {0, 0, 0, 255};

	if (which < MAX_COLORS && which >= 0)
		color = settings->colors[which];

	return color;
}

void
BowserApp::SetColor (int32 which, const rgb_color color)
{
	if (which < MAX_COLORS &&  which >= 0
	&& (settings->colors[which].red   != color.red
	||  settings->colors[which].green != color.green
	||  settings->colors[which].blue  != color.blue
	||  settings->colors[which].alpha != color.alpha))
	{
		settings->colors[which] = color;

		BMessage msg (M_STATE_CHANGE);

		msg.AddInt32 ("which", which);
		msg.AddData (
			"color",
			B_RGB_COLOR_TYPE,
			settings->colors + which,
			sizeof (rgb_color));
		Broadcast (&msg);
	}
}

BString
BowserApp::GetEvent (int32 which) const
{
	BString buffer;

	if (which < MAX_EVENTS && which >= 0)
		buffer = settings->events[which];

	return buffer;
}

void
BowserApp::SetEvent (int32 which, const char *event)
{
	if (which < MAX_EVENTS && which >= 0
	&&  settings->events[which].Compare (event))
	{
		settings->events[which] = event;

		BMessage msg (M_STATE_CHANGE);

		msg.AddInt32 ("which", which);
		msg.AddString ("event", settings->events[which].String());
		Broadcast (&msg);
	}
}

BString
BowserApp::GetCommand (int32 which)
{
	BAutolock lock (this);
	BString buffer;

	if (which < MAX_COMMANDS && which >= 0 && lock.IsLocked())
		buffer = settings->commands[which];

	return buffer;
}

void
BowserApp::SetCommand (int32 which, const char *command)
{
	BAutolock lock (this);

	if (which < MAX_EVENTS && which >= 0 && lock.IsLocked())
	{
		settings->commands[which] = command;

		// No need to broadcast.  It's pretty darn unlikely that
		// a user can change the pref and in the same instance
		// issue a part, quit, or kick command
		// although ignore is more likely -- since it's rare
		// we'll keep it as is
	}

}


void
BowserApp::NicknameBindState (bool state)
{
	if (settings->nicknameBindState != state
	&&  settings->nicknameBindState)
	{
		bool hit (false);

		// First, since we're going to one list for
		// all servers, add the nicks that are not in
		// each of the servers' order lists
		for (int32 i = 0; i < settings->servers.CountItems(); ++i)
		{
			ServerData *sd ((ServerData *)settings->servers.ItemAt (i));

			for (uint32 j = 0; j < settings->nicks.size(); ++j)
				if (AddNickname (sd, settings->nicks[j].String()))
					hit = true;
		}

		if (hit)
		{
			// This one, we know only setupWindow is interested
			BMessage msg (M_STATE_CHANGE);

			msg.AddBool ("nickbind", state);
			setupWindow->PostMessage (&msg);
		}
	}

	settings->nicknameBindState = state;
}

bool
BowserApp::GetNicknameBindState (void) const
{
	return settings->nicknameBindState;
}

uint32
BowserApp::GetNicknameCount (void) const
{
	return settings->nicks.size();
}

BString
BowserApp::GetNickname (uint32 which) const
{
	BString result;

	if (which < settings->nicks.size())
		result = settings->nicks[which];

	return result;
}

bool
BowserApp::AddNickname (ServerData *sd, const char *nick)
{
	// make sure we don't already have it

	for (uint32 i = 0; i < settings->nicks.size(); ++i)
		if (settings->nicks[i] == nick)
		{
			if ((settings->nicknameBindState && !sd)
			||  (settings->nicknameBindState
			&&   find (sd->order.begin(), sd->order.end(), i) != sd->order.end())
			||  !settings->nicknameBindState)
				return false;

			sd->order.push_back (i);
			return true;
		}

	// if we are not bound to server,
	// tack on at the end of each server order
	if (!settings->nicknameBindState)
		for (int32 i = 0; i < settings->servers.CountItems(); ++i)
		{
			ServerData *serverData ((ServerData *)settings->servers.ItemAt (i));

			serverData->order.push_back (settings->nicks.size());
		}
	else if (sd)
		sd->order.push_back (settings->nicks.size());
		
		
	settings->nicks.push_back (BString (nick));

	return true;
}

bool
BowserApp::RemoveNickname (ServerData *sd, uint32 which)
{
	int32 count (0);
	bool work (false);

	if (which < settings->nicks.size()
	&& (!settings->nicknameBindState
	||  find (sd->order.begin(), sd->order.end(), which) != sd->order.end()))
	{
		work = true;

		if (sd)
			sd->order.erase (find (sd->order.begin(), sd->order.end(), which));

		// Count number of servers that have this nickname
		// OR remove from all order lists
		for (int32 i = 0; i < settings->servers.CountItems(); ++i)
		{
			ServerData *serverData ((ServerData *)settings->servers.ItemAt (i));
	
			if (settings->nicknameBindState
			&&  find (serverData->order.begin(), serverData->order.end(), which)
					!= serverData->order.end())
				++count;
			else if (!settings->nicknameBindState && serverData != sd)
				serverData->order.erase (
					find (serverData->order.begin(),
					serverData->order.end(),
					which));
		}

		// Now, remove from nickname list
		if (count == 0)
		{
			settings->nicks.erase (settings->nicks.begin() + which);

			// We have to adjust all the servers order since we yanked one
			for (int32 i = 0; i < settings->servers.CountItems(); ++i)
			{
				ServerData *serverData ((ServerData *)settings->servers.ItemAt (i));

				for (uint32 j = 0; j < serverData->order.size(); ++j)
					if (serverData->order[j] > which)
						serverData->order[j] -= 1;
			}
		}
	}

	return count == 0 && work;
}

bool
BowserApp::CanNotify (void) const
{
	return settings && settings->deskbarMsgr.IsValid();
}

void
BowserApp::Broadcast (BMessage *msg)
{
	Lock();

	for (int32 i = 0; i < CountWindows(); ++i)
		WindowAt (i)->PostMessage (msg);

	Unlock();
}

void
BowserApp::Broadcast (BMessage *msg, const char *serverName, bool active)
{
	Lock();

	for (int32 i = 0; i < CountWindows(); ++i)
	{
		ServerWindow *serverWindow (dynamic_cast<ServerWindow *>(WindowAt (i)));

		if (serverWindow
		&&  serverWindow->Id().ICompare (serverName) == 0)
			if (active)
				serverWindow->PostActive (msg);
			else
				serverWindow->RepliedBroadcast (msg);
	}

	Unlock();
}

ServerData *
BowserApp::FindServerData (const char *serverName)
{
	for (int32 i = 0; i < settings->servers.CountItems(); ++i)
	{
		ServerData *item ((ServerData *)settings->servers.ItemAt (i));

		if (item->server.ICompare (serverName) == 0)
			return item;
	}

	return 0;
}

AppSettings::AppSettings (void)
	: Settings ("app", BOWSER_SETTINGS),

	  stampState (false),
	  paranoidState (false),
	  nicknameBindState (true),
	  messageOpenState (true),
	  messageFocusState (true),
	  windowFollowsState (true),
	  hideSetupState (false),
	  showSetupState (false),
	  showTopicState (true),
	  statusTopicState (false),
	  altwSetupState (true),
	  altwServerState (true),
	  masterLogState (false),
	  dateLogsState (false),
	  alsoKnownAs ("bowserUser beosUser"),
	  otherNick ("tlair"),
	  autoNickTime ("10"),
	  notificationMask (
	  	NOTIFY_NICK_BIT |
		NOTIFY_CONT_BIT |
		NOTIFY_NICK_FLASH_BIT |
		NOTIFY_CONT_FLASH_BIT),
	  current (0),
	  setupFrame (100, 100, 485, 290)
{
	// Font Preferences (0 - text, 1 - server, 2 - url 3-names
	client_font[F_TEXT]   = new BFont (be_plain_font);
	client_font[F_SERVER] = new BFont (be_fixed_font);
	client_font[F_URL]    = new BFont (be_bold_font);
	client_font[F_NAMES]  = new BFont (be_plain_font);
	client_font[F_INPUT]  = new BFont (be_plain_font);

	for (int32 i = 0; i < MAX_FONTS; ++i)
	{
		// In case someone adds some half way
		if (i > F_TEXT
		&&  i > F_SERVER
		&&  i > F_URL
		&&  i > F_NAMES
		&&  i > F_INPUT)
			client_font[i] = new BFont (be_plain_font);
		client_font[i]->SetEncoding (B_UNICODE_UTF8);
	}

	
	const rgb_color myBlack				= {0,0,0, 255};
	const rgb_color myWhite				= {255, 255, 255, 255};
	const rgb_color NOTICE_COLOR		= {10,90,170, 255};
	const rgb_color ACTION_COLOR		= {225,10,10, 255};
	const rgb_color QUIT_COLOR			= {180,10,10, 255};
	const rgb_color ERROR_COLOR		= {210,5,5, 255};
	const rgb_color URL_COLOR			= {5,5,150, 255};
	const rgb_color NICK_COLOR			= {10,10,190, 255};
	const rgb_color MYNICK_COLOR		= {200,10,20, 255};
	const rgb_color JOIN_COLOR			= {10,130,10, 255};
	const rgb_color KICK_COLOR			= {250,130,10, 255};
	const rgb_color WHOIS_COLOR		= {10,30,170, 255};
	const rgb_color OP_COLOR			= {140,10,40, 255};
	const rgb_color VOICE_COLOR		= {160, 20, 20, 255};
	const rgb_color CTCP_REQ_COLOR	= {10,10,180, 255};
	const rgb_color CTCP_RPY_COLOR	= {10,40,180, 255};
	const rgb_color IGNORE_COLOR		= {100, 100, 100, 255};
	const rgb_color INPUT_COLOR		= {0, 0, 0, 255};
	const rgb_color	INPUT_BG_COLOR	= {255, 255, 255, 255};

	colors[C_TEXT]							= myBlack;
	colors[C_BACKGROUND]					= myWhite;
	colors[C_NAMES]						= myBlack;
	colors[C_NAMES_BACKGROUND]			= myWhite;
	colors[C_URL]							= URL_COLOR;
	colors[C_SERVER]						= myBlack;
	colors[C_NOTICE]						= NOTICE_COLOR;
	colors[C_ACTION]						= ACTION_COLOR;
	colors[C_QUIT]							= QUIT_COLOR;
	colors[C_ERROR]						= ERROR_COLOR;
	colors[C_NICK]							= NICK_COLOR;
	colors[C_MYNICK]						= MYNICK_COLOR;
	colors[C_JOIN]							= JOIN_COLOR;
	colors[C_KICK]							= KICK_COLOR;
	colors[C_WHOIS]						= WHOIS_COLOR;
	colors[C_OP]							= OP_COLOR;
	colors[C_VOICE]						= VOICE_COLOR;
	colors[C_CTCP_REQ]					= CTCP_REQ_COLOR;
	colors[C_CTCP_RPY]					= CTCP_RPY_COLOR;
	colors[C_IGNORE]						= IGNORE_COLOR;
	colors[C_INPUT]						= INPUT_COLOR;
	colors[C_INPUT_BACKGROUND]			= INPUT_BG_COLOR;

	events[E_JOIN]							= "*** $N ($I@$A) has joined the channel.";
	events[E_PART]							= "*** $N has left the channel.";
	events[E_NICK]							= "*** $N is now known as $n.";
	events[E_QUIT]							= "*** $N ($I@$A) has quit IRC ($R)";
	events[E_KICK]							= "*** $N has been kicked from $C by $n ($R)";
	events[E_TOPIC]							= "*** $C Topic changed by $N: $T";
	events[E_SNOTICE]						= "$R";
	events[E_UNOTICE]						= "-$N- $R";
	events[E_NOTIFY_ON]						= "*** $N has joined IRC.";
	events[E_NOTIFY_OFF]					= "*** $N has left IRC.";

	commands[CMD_KICK]					= "Ouch!";
	commands[CMD_QUIT]					= "Bowser[$V]: server window terminating...";
	commands[CMD_IGNORE]				= "*** $N is now ignored ($i).";
	commands[CMD_UNIGNORE]				= "*** $N is no longer ignored.";
	commands[CMD_AWAY]					= "is idle: $R";
	commands[CMD_BACK]					= "has returned";
	commands[CMD_UPTIME]				= "System was last booted $U ago.";
}

AppSettings::~AppSettings (void)
{
	for (int32 i = 0; i < MAX_FONTS; ++i)
		if (client_font[i])
			delete client_font[i];

	nicks.erase (nicks.begin(), nicks.end());

	RemoveDeskbarIcon();
}

void
AppSettings::RestorePacked (BMessage *msg)
{
	if  (msg->HasBool ("TimeStamp"))
		msg->FindBool ("TimeStamp", &stampState);
		
	if (msg->HasBool ("Paranoid"))
		msg->FindBool ("Paranoid", &paranoidState);

	if (msg->HasInt32 ("NotificationMask"))
		msg->FindInt32 ("NotificationMask", &notificationMask);

	if (msg->HasBool ("NicknameBind"))
		msg->FindBool ("NicknameBind", &nicknameBindState);

	if (msg->HasString ("AlsoKnownAs"))
	{
		const char *buffer;

		msg->FindString ("AlsoKnownAs", &buffer);
		alsoKnownAs = buffer;
	}

	if (msg->HasString ("Other Nick"))
	{
		const char *buffer;

		msg->FindString ("Other Nick", &buffer);
		otherNick = buffer;
	}

	if (msg->HasString ("Auto Nick Time"))
	{
		const char *buffer;

		msg->FindString ("Auto Nick Time", &buffer);
		autoNickTime = buffer;
	}

	for (int32 i = 0; msg->HasString ("Nickname", i); ++i)
	{
		const char *buffer;

		msg->FindString ("Nickname", i, &buffer);
		nicks.push_back (BString (buffer));
	}

	for (int32 i = 0; msg->HasData ("Color", B_RGB_COLOR_TYPE, i); ++i)
	{
		const rgb_color *c;
		ssize_t size;

		msg->FindData (
			"Color",
			B_RGB_COLOR_TYPE,
			i,
			reinterpret_cast<const void **>(&c),
			&size);

		colors[i] = *c;
	}

	for (int32 i = 0; msg->HasString ("Event", i); ++i)
	{
		const char *buffer;

		msg->FindString ("Event", i, &buffer);
		events[i] = buffer;
	}

	for (int32 i = 0; msg->HasString ("Command", i); ++i)
	{
		const char *buffer;

		msg->FindString ("Command", i, &buffer);
		commands[i] = buffer;
	}

	if (msg->HasBool ("Message Open"))
		msg->FindBool ("Message Open", &messageOpenState);

	if (msg->HasBool ("Message Focus"))
		msg->FindBool ("Message Focus", &messageFocusState);

	if (msg->HasBool ("Window Follows"))
		msg->FindBool ("Window Follows", &windowFollowsState);

	if (msg->HasBool ("Hide Setup"))
		msg->FindBool ("Hide Setup", &hideSetupState);

	if (msg->HasBool ("Show Setup"))
		msg->FindBool ("Show Setup", &showSetupState);
		
	if (msg->HasBool ("Show Topic"))
		msg->FindBool ("Show Topic", &showTopicState);
		
	if (msg->HasBool ("Status Topic"))
		msg->FindBool ("Status Topic", &statusTopicState);

	if (msg->HasBool ("AltW Setup"))
		msg->FindBool ("AltW Setup", &altwSetupState);
		
	if (msg->HasBool ("AltW Server"))
		msg->FindBool ("AltW Server", &altwServerState);

	if (msg->HasBool ("Master Log"))
		msg->FindBool ("Master Log", &masterLogState);

	if (msg->HasBool ("Date Logs"))
		msg->FindBool ("Date Logs", &dateLogsState);


	for (int32 i = 0; i < MAX_FONTS; ++i)
	{
		if (msg->HasString ("Font Family", i))
		{
			const char *family, *style;
			float size;

			msg->FindString ("Font Family", i, &family);
			msg->FindString ("Font Style",  i, &style);
			msg->FindFloat  ("Font Size",   i, &size);

			client_font[i]->SetFamilyAndStyle (family, style);
			client_font[i]->SetSize (size);
		}
	}

	if (msg->HasInt32 ("current"))
		msg->FindInt32 ("current", &current);

	if (msg->HasRect ("frame"))
		msg->FindRect ("frame", &setupFrame);

	for (int32 i = 0; msg->HasString ("server", i); ++i)
	{
		ServerData *serverData (new ServerData);
		const char *buffer;

		msg->FindString ("server", i, &buffer);
		serverData->server = buffer;

		if (msg->HasString ("port", i))
		{
			msg->FindString ("port", i, &buffer);
			serverData->port = buffer;
		}

		if (msg->HasString ("name", i))
		{
			msg->FindString ("name", i, &buffer);
			serverData->name = buffer;
		}

		if (msg->HasString ("ident", i))
		{
			msg->FindString ("ident", i, &buffer);
			serverData->ident = buffer;
		}

		if (msg->HasBool ("motd", i))
			msg->FindBool ("motd", i, &serverData->motd);

		if (msg->HasBool ("identd", i))
			msg->FindBool ("identd", i, &serverData->identd);
		
		if (msg->HasBool ("connect", i))
			msg->FindBool ("connect", i, &serverData->connect);

		if (msg->HasString ("cmds", i))
		{
			msg->FindString ("cmds", i, &buffer);
			serverData->connectCmds = buffer;
		}

		if (msg->HasMessage ("order", i))
		{
			BMessage buffer;

			msg->FindMessage ("order", i, &buffer);

			for (int32 j = 0; buffer.HasInt32 ("index", j); ++j)
			{
				int32 index;

				buffer.FindInt32 ("index", j, &index);
				serverData->order.push_back (index);
			}
		}

		if (msg->HasMessage ("ignores", i))
		{
			BMessage buffer;

			msg->FindMessage ("ignores", i, &buffer);

			for (int32 j = 0; buffer.HasString ("regex", j); ++j)
			{
				IgnoreItem *item;
				const char *re;

				buffer.FindString ("regex", j, &re);
				
				item = new IgnoreItem (re);

				if (buffer.HasMessage ("exclude", j))
				{
					BMessage exclude;

					buffer.FindMessage ("exclude", j, &exclude);

					for (int32 k = 0; exclude.HasString ("nick", k); ++k)
					{
						const char *nick;

						exclude.FindString ("nick", k, &nick);
						item->AddExclude (nick);
					}
				}

				serverData->ignores.AddItem (item);
			}
		}

		if (msg->HasMessage ("notifies", i))
		{
			BMessage buffer;

			msg->FindMessage ("notifies", i, &buffer);

			for (int32 j = 0; buffer.HasString ("nick", j); ++j)
			{
				const char *nick;
				NotifyItem *item;
				
				buffer.FindString ("nick", j, &nick);
				item = new NotifyItem (nick, false);

				serverData->notifies.AddItem (item);
			}
		}

		servers.AddItem (serverData);
	}

	if (servers.IsEmpty())
	{
		ServerData *serverData (new ServerData);
	
		serverData->ident  = "bowser";
		serverData->name   = "Bowser User";
		serverData->server = "irc.reefer.org";
		serverData->port   = "6667";

		for (uint32 i = 0; i < nicks.size(); ++i)
			serverData->order.push_back (i);
	
		current = 0;
		servers.AddItem (serverData);
	}

	AddDeskbarIcon (notificationMask);
}

void
AppSettings::AddDeskbarIcon (int32 mask)
{
	if ((mask & NOTIFY_NICK_BIT) != 0
	||  (mask & NOTIFY_CONT_BIT) != 0)
	{
		#ifdef __INTEL__
		BDeskbar deskbar;
		#else
		BDeskbarInterface deskbar;
		#endif

		if (

		#ifdef DEV_BUILD
		    !NotifyReuse ||
		#endif

		   !deskbar.HasItem ("Bowser:Notify")
		||  deskbar.GetItemInfo ("Bowser:Notify", &deskbarId) != B_NO_ERROR)
		{
			NotifyView *notify;

			notify = new NotifyView (mask);
			deskbar.AddItem (notify, &deskbarId);

			delete notify;
		}

		// Get messenger for view in deskbar
		BMessenger msgr ("application/x-vnd.Be-TSKB");

		if (msgr.IsValid())
		{
			BMessage msg (B_GET_PROPERTY),
						mid (B_ID_SPECIFIER),
						reply;
			mid.AddString ("property", "Replicant");
			mid.AddInt32 ("id", deskbarId);

			msg.AddSpecifier ("Messenger");
			msg.AddSpecifier ("View");
			msg.AddSpecifier (&mid);
			msg.AddSpecifier ("Shelf");
			msg.AddSpecifier ("View", "Status");
			msg.AddSpecifier ("Window", "Deskbar");
	
			msgr.SendMessage (&msg, &reply);
	
			int32 error (B_NO_ERROR);
	
			if (reply.HasInt32 ("error"))
				reply.FindInt32 ("error", &error);
	
			if (error == B_NO_ERROR)
			{
				reply.FindMessenger ("result", &deskbarMsgr);
	
				#ifdef DEV_BUILD
				if (NotifyStatus && deskbarMsgr.IsValid())
				{
					printf ("*** All is well with Notify on Bowser's end\n");
				}
				#endif
			}
		}
	}
}

void
AppSettings::RemoveDeskbarIcon (void)
{
	if (deskbarMsgr.IsValid())
	{
		BDeskbar deskbar;
		status_t status;

		status = deskbar.RemoveItem (deskbarId);

		#ifdef DEV_BUILD
		if (NotifyStatus && status != B_NO_ERROR)
			printf ("*** Could not remove: %s\n", strerror (status));
		#endif

		// Just to be sure (Invalidate it)
		deskbarMsgr = BMessenger();
	}
}

void
AppSettings::SavePacked (BMessage *msg)
{
	msg->AddBool ("TimeStamp", stampState);
	msg->AddBool ("Paranoid", paranoidState);
	msg->AddBool ("NicknameBind", nicknameBindState);
	msg->AddBool ("Message Open", messageOpenState);
	msg->AddBool ("Message Focus", messageFocusState);
	msg->AddBool ("Window Follows", windowFollowsState);
	msg->AddBool ("Hide Setup", hideSetupState);
	msg->AddBool ("Show Setup", showSetupState);
    msg->AddBool ("Show Topic", showTopicState);
    msg->AddBool ("Status Topic", statusTopicState);
    msg->AddBool ("AltW Setup", altwSetupState);
    msg->AddBool ("AltW Server", altwServerState);
    msg->AddBool ("Master Log", masterLogState);
    msg->AddBool ("Date Logs", dateLogsState);
	msg->AddInt32 ("NotificationMask", notificationMask);

	msg->AddString ("AlsoKnownAs", alsoKnownAs.String());
	msg->AddString ("Other Nick", otherNick.String());
	msg->AddString ("Auto Nick Time", autoNickTime.String());

	for (uint32 i = 0; i < nicks.size(); ++i)
		msg->AddString ("Nickname", nicks[i].String());

	for (int32 i = 0; i < MAX_COLORS; ++i)
		msg->AddData (
			"Color",
			B_RGB_COLOR_TYPE,
			colors + i,
			sizeof (rgb_color));

	for (int32 i = 0; i < MAX_EVENTS; ++i)
		msg->AddString ("Event", events[i].String());

	for (int32 i = 0; i < MAX_COMMANDS; ++i)
		msg->AddString ("Command", commands[i].String());

	// Font preferences
	for (int32 i = 0; i < MAX_FONTS; ++i)
	{
		if (client_font[i])
		{
			font_family family;
			font_style style;
			float size;

			client_font[i]->GetFamilyAndStyle (&family, &style);
			size = client_font[i]->Size();

			msg->AddString ("Font Family", family);
			msg->AddString ("Font Style", style);
			msg->AddFloat ("Font Size", size);
		}
		else break;
	}

	msg->AddInt32 ("current", current);
	msg->AddRect ("frame", setupFrame);

	for (int32 i = 0; i < servers.CountItems(); ++i)
	{
		ServerData *serverData ((ServerData *)servers.ItemAt (i));

		msg->AddString ("server", serverData->server.String());
		msg->AddString ("port", serverData->port.String());
		msg->AddString ("name", serverData->name.String());
		msg->AddString ("ident", serverData->ident.String());
		msg->AddBool ("motd", serverData->motd);
		msg->AddBool ("identd", serverData->identd);
		msg->AddBool ("connect", serverData->connect);
		msg->AddString ("cmds", serverData->connectCmds.String());

		BMessage buffer;

		for (uint32 j = 0; j < serverData->order.size(); ++j)
			buffer.AddInt32 ("index", serverData->order[j]);
		msg->AddMessage ("order", &buffer);

		buffer.MakeEmpty();
		for (int32 j = 0; j < serverData->ignores.CountItems(); ++j)
		{
			IgnoreItem *item ((IgnoreItem *)serverData->ignores.ItemAt (j));
			BMessage exclude;

			buffer.AddString ("regex", item->Text());

			for (int32 k = 0; k < item->CountExcludes(); ++k)
			{
				BStringItem *sItem (item->ExcludeAt (k));

				exclude.AddString ("nick", sItem->Text());
			}

			buffer.AddMessage ("exclude", &exclude);
		}
		msg->AddMessage ("ignores", &buffer);

		buffer.MakeEmpty();
		for (int32 j = 0; j < serverData->notifies.CountItems(); ++j)
		{
			NotifyItem *item ((NotifyItem *)serverData->notifies.ItemAt (j));

			buffer.AddString ("nick", item->Text());
		}
		msg->AddMessage ("notifies", &buffer);
	}
}


