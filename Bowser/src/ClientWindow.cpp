#include <FilePanel.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <ScrollView.h>
#include <View.h>
#include <TextControl.h>
#include <Roster.h>
#include <FindDirectory.h>
#include <map>
#include <ctype.h>
#include <netdb.h>

#include "Bowser.h"
#include "IRCDefines.h"
#include "MessageWindow.h"
#include "HistoryMenu.h"
#include "Names.h"
#include "IRCView.h"
#include "StringManip.h"
#include "Settings.h"
#include "StatusView.h"
#include "ServerWindow.h"
#include "ClientFilter.h"
#include "ClientWindow.h"
#include "ChannelWindow.h"
#include "IgnoreWindow.h"

#include <stdio.h>

// For the sole purpose of keeping the template
// restricted to this file
struct CommandWrapper
{
	map<BString, ClientWindow::CmdFunc>				cmds;
};

const char *ClientWindow::endl						("\1\1\1\1\1");

ClientWindow::ClientWindow (
	const char *id_,
	int32 sid_,
	const char *serverName_,
	const BMessenger &sMsgr_,
	const char *myNick_,
	BRect frame,
	WindowSettings *settings_)

	: BWindow (
		frame,
		id_,
		B_DOCUMENT_WINDOW,
			B_ASYNCHRONOUS_CONTROLS
			| (bowser_app->GetWindowFollowsState()
				?  (B_NOT_ANCHORED_ON_ACTIVATE | B_NO_WORKSPACE_ACTIVATION)
				:  0)),

	  id (id_),
	  sid (sid_),
	  serverName (serverName_),
	  sMsgr (sMsgr_),
	  myNick (myNick_),
	  timeStampState (bowser_app->GetStampState()),
	  settings (settings_),
	  cmdWrap (0)
{
	Init();
}

ClientWindow::ClientWindow (
	const char *id_,
	int32 sid_,
	const char *serverName_,
	const char *myNick_,
	BRect frame,
	WindowSettings *settings_)

	: BWindow (
		frame,
		id_,
		B_DOCUMENT_WINDOW,
			B_ASYNCHRONOUS_CONTROLS
			| (bowser_app->GetWindowFollowsState()
				?  (B_NOT_ANCHORED_ON_ACTIVATE | B_NO_WORKSPACE_ACTIVATION)
				:  0)),

	  id (id_),
	  sid (sid_),
	  serverName (serverName_),
	  myNick (myNick_),
	  timeStampState (bowser_app->GetStampState()),
	  settings (settings_),
	  cmdWrap (0)
{
	Init();
	sMsgr = BMessenger (this);
}

void
ClientWindow::Init (void)
{
	BRect frame (Bounds());
	menubar = new BMenuBar (frame, "menu_bar");

	BMenuItem *item;
	BMessage *msg;

	mServer = new BMenu ("Server");

	msg = new BMessage (M_HISTORY_SELECT);
	mServer->AddItem (mHistory = new BMenuItem ("Input History", msg, 'H'));

	msg = new BMessage (M_IGNORE_WINDOW);
	msg->AddString ("server", serverName.String());
	msg->AddRect ("frame", Frame());
	mServer->AddItem (mIgnore = new BMenuItem ("Ignored List" B_UTF8_ELLIPSIS,
		msg, 'I'));

	msg = new BMessage (M_LIST_COMMAND);
	msg->AddString ("server", serverName.String());
	msg->AddRect ("frame", Frame());
	mServer->AddItem (mList = new BMenuItem ("Channel List" B_UTF8_ELLIPSIS,
		msg, 'L'));

	msg = new BMessage (M_NOTIFY_WINDOW);
	msg->AddString ("server", serverName.String());
	msg->AddRect ("frame", Frame());
	mServer->AddItem (mNotifyWindow = new BMenuItem ("Notify List"
		B_UTF8_ELLIPSIS, msg, 'N'));

	mServer->AddSeparatorItem();
	mServer->AddItem (item = new BMenuItem ("Quit",
		new BMessage (B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem (mServer);
	item->SetTarget (bowser_app);
	mList->SetTarget (bowser_app);
	mNotifyWindow->SetTarget (bowser_app);
	mIgnore->SetTarget (bowser_app);

	mWindow = new BMenu ("Window");
	msg = new BMessage (CYCLE_WINDOWS);
	msg->AddPointer ("client", this);
	mWindow->AddItem (item = new BMenuItem ("Next Window", msg, ','));
	item->SetTarget (sMsgr);

	msg = new BMessage (CYCLE_BACK);
	msg->AddPointer ("client", this);
	mWindow->AddItem (item = new BMenuItem ("Previous Window", msg, '.'));
	item->SetTarget (sMsgr);

	BLooper *sLooper;
	sMsgr.Target (&sLooper);
	msg = new BMessage (CYCLE_WINDOWS);
	msg->AddPointer ("server", sLooper ? sLooper : this);
	mWindow->AddItem (item = new BMenuItem ("Next Server", msg, ',',
		B_SHIFT_KEY));
	item->SetTarget (bowser_app);

	msg = new BMessage (CYCLE_BACK);
	msg->AddPointer ("server", sLooper ? sLooper : this);
	mWindow->AddItem (item = new BMenuItem ("Previous Server", msg, '.',
		B_SHIFT_KEY));
	item->SetTarget (bowser_app);

	mWindow->AddSeparatorItem();

	msg = new BMessage (M_SETUP_ACTIVATE);
	mWindow->AddItem (item = new BMenuItem ("Setup", msg, '/', B_SHIFT_KEY));
	item->SetTarget (bowser_app);

	msg = new BMessage (M_PREFS_BUTTON);
	mWindow->AddItem (item = new BMenuItem ("Preferences" B_UTF8_ELLIPSIS, msg,
		'P'));
	item->SetTarget (bowser_app);
	menubar->AddItem (mWindow);

	mWindow->AddSeparatorItem();
	msg = new BMessage (M_SCROLL_TOGGLE);
	mWindow->AddItem (mScrolling = new BMenuItem ("Auto Scrolling", msg, 'S'));
	mScrolling->SetMarked (true);

	mHelp = new BMenu ("Help");
	mHelp->AddItem (item = new BMenuItem ("About Bowser",
		new BMessage (B_ABOUT_REQUESTED), 'A', B_SHIFT_KEY));
	item->SetTarget (bowser_app);
	menubar->AddItem (mHelp);
	AddChild (menubar);
	
	frame.top = menubar->Frame().bottom + 1;
	bgView = new BView (frame,
		"Background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW);

	bgView->SetViewColor (212, 212, 212);
	AddChild (bgView);

	frame = bgView->Bounds();

	status = new StatusView (frame);
	bgView->AddChild (status);

	inputFont   = *(bowser_app->GetClientFont (F_INPUT));
	input = new BTextControl (
		BRect (2, frame.top, frame.right - 15, frame.bottom),
		"Input", 0, 0,
		0,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	input->SetDivider (0);
	input->TextView()->SetFontAndColor (&inputFont);
	input->ResizeToPreferred();
	input->MoveTo (
		2,
		status->Frame().top - input->Frame().Height() - 1);

	input->TextView()->AddFilter (new ClientInputFilter (this));
	bgView->AddChild (input);

	history = new HistoryMenu (BRect (
		frame.right - 11,
		input->Frame().top,
		frame.right - 1,
		input->Frame().bottom - 1));
	bgView->AddChild (history);

	frame.bottom = input->Frame().top - 3;
	frame.right -= B_V_SCROLL_BAR_WIDTH;

	text = new IRCView (
		frame,
		frame.InsetByCopy (3.0, 3.0),
		input);

	text->SetViewColor (bowser_app->GetColor (C_BACKGROUND));

	scroll = new BScrollView (
		"scroll",
		text,
		B_FOLLOW_ALL_SIDES,
		0,
		false,
		true,
		B_PLAIN_BORDER);
	bgView->AddChild (scroll);

	// For dynamic menus
	mNotification[0] = mNotification[1] = mNotification[2] = 0;

	// A client window maintains it's own state, so we don't
	// have to lock the app everytime we want to get this information.
	// When it changes, the application looper will let us know
	// through a M_STATE_CHANGE message
	textColor		= bowser_app->GetColor (C_TEXT);
	nickColor		= bowser_app->GetColor (C_NICK);
	ctcpReqColor	= bowser_app->GetColor (C_CTCP_REQ);
	quitColor		= bowser_app->GetColor (C_QUIT);
	whoisColor		= bowser_app->GetColor (C_WHOIS);
	myNickColor		= bowser_app->GetColor (C_MYNICK);
	actionColor		= bowser_app->GetColor (C_ACTION);
	opColor			= bowser_app->GetColor (C_OP);

	myFont			= *(bowser_app->GetClientFont (F_TEXT));
	serverFont		= *(bowser_app->GetClientFont (F_SERVER));
	canNotify		= bowser_app->CanNotify();
	alsoKnownAs		= bowser_app->GetAlsoKnownAs();
	otherNick		= bowser_app->GetOtherNick();
	autoNickTime	= bowser_app->GetAutoNickTime();
	notifyMask		= bowser_app->GetNotificationMask();

	scrolling    = true;


	cmdWrap = new CommandWrapper;

	// Add Commands (make cmdWrap static?)
	cmdWrap->cmds["/PING"]			= &ClientWindow::PingCmd;
	cmdWrap->cmds["/VERSION"]		= &ClientWindow::VersionCmd;
	cmdWrap->cmds["/NOTICE"]		= &ClientWindow::NoticeCmd;
	cmdWrap->cmds["/ADMIN"]			= &ClientWindow::AdminCmd;
	cmdWrap->cmds["/INFO"]			= &ClientWindow::InfoCmd;
	cmdWrap->cmds["/RAW"]			= &ClientWindow::RawCmd;
	cmdWrap->cmds["/QUOTE"]			= &ClientWindow::RawCmd;
	cmdWrap->cmds["/STATS"]			= &ClientWindow::StatsCmd;
	cmdWrap->cmds["/OPER"]			= &ClientWindow::OperCmd;
	cmdWrap->cmds["/REHASH"]		= &ClientWindow::RehashCmd;
	cmdWrap->cmds["/WALLOPS"]		= &ClientWindow::WallopsCmd;
	cmdWrap->cmds["/KILL"]			= &ClientWindow::KillCmd;
	cmdWrap->cmds["/TRACE"]			= &ClientWindow::TraceCmd;
	cmdWrap->cmds["/QUIT"]			= &ClientWindow::QuitCmd;
	cmdWrap->cmds["/AWAY"]			= &ClientWindow::AwayCmd;
	cmdWrap->cmds["/ME"]			= &ClientWindow::MeCmd;
	cmdWrap->cmds["/DESCRIBE"]		= &ClientWindow::DescribeCmd;
	cmdWrap->cmds["/JOIN"]			= &ClientWindow::JoinCmd;
	cmdWrap->cmds["/NICK"]			= &ClientWindow::NickCmd;
	cmdWrap->cmds["/MSG"]			= &ClientWindow::MsgCmd;
	cmdWrap->cmds["/CTCP"]			= &ClientWindow::CtcpCmd;
	cmdWrap->cmds["/KICK"]			= &ClientWindow::KickCmd;
	cmdWrap->cmds["/WHOIS"]			= &ClientWindow::WhoIsCmd;
	cmdWrap->cmds["/OP"]			= &ClientWindow::OpCmd;
	cmdWrap->cmds["/DEOP"]			= &ClientWindow::DopCmd;
	cmdWrap->cmds["/DOP"]			= &ClientWindow::DopCmd;
	cmdWrap->cmds["/MODE"]			= &ClientWindow::ModeCmd;
	cmdWrap->cmds["/MOTD"]			= &ClientWindow::MotdCmd;
	cmdWrap->cmds["/T"]				= &ClientWindow::TopicCmd;
	cmdWrap->cmds["/TOPIC"]			= &ClientWindow::TopicCmd;
	cmdWrap->cmds["/NAMES"]			= &ClientWindow::NamesCmd;
	cmdWrap->cmds["/Q"]				= &ClientWindow::QueryCmd;
	cmdWrap->cmds["/QUERY"]			= &ClientWindow::QueryCmd;
	cmdWrap->cmds["/WHO"]			= &ClientWindow::WhoCmd;
	cmdWrap->cmds["/WHOWAS"]		= &ClientWindow::WhoWasCmd;
	cmdWrap->cmds["/DCC"]			= &ClientWindow::DccCmd;
	cmdWrap->cmds["/INVITE"]		= &ClientWindow::InviteCmd;
	cmdWrap->cmds["/LIST"]			= &ClientWindow::ListCmd;
	cmdWrap->cmds["/IGNORE"]		= &ClientWindow::IgnoreCmd;
	cmdWrap->cmds["/UNIGNORE"]		= &ClientWindow::UnignoreCmd;
	cmdWrap->cmds["/EXCLUDE"]		= &ClientWindow::ExcludeCmd;
	cmdWrap->cmds["/NOTIFY"]		= &ClientWindow::NotifyCmd;
	cmdWrap->cmds["/UNNOTIFY"]		= &ClientWindow::UnnotifyCmd;
	cmdWrap->cmds["/J"]				= &ClientWindow::JoinCmd;
	cmdWrap->cmds["/M"]				= &ClientWindow::Mode2Cmd;
	cmdWrap->cmds["/W"]				= &ClientWindow::WhoIsCmd;
	cmdWrap->cmds["/K"]				= &ClientWindow::KickCmd;
	cmdWrap->cmds["/SLEEP"]			= &ClientWindow::SleepCmd;
	cmdWrap->cmds["/VISIT"]			= &ClientWindow::VisitCmd;
	cmdWrap->cmds["/NICKSERV"]		= &ClientWindow::NickServCmd;
	cmdWrap->cmds["/CHANSERV"]		= &ClientWindow::ChanServCmd;
	cmdWrap->cmds["/MEMOSERV"]		= &ClientWindow::MemoServCmd;
	cmdWrap->cmds["/USERHOST"]		= &ClientWindow::UserhostCmd;
	cmdWrap->cmds["/UPTIME"]		= &ClientWindow::UptimeCmd;
	cmdWrap->cmds["/DNS"]			= &ClientWindow::DnsCmd;

	
	// no parms
	// TODO:	add wrapper that doesn't send const char *
	//			(will fix warnings)
	cmdWrap->cmds["/CLEAR"]			= &ClientWindow::ClearCmd;
	cmdWrap->cmds["/PART"]			= &ClientWindow::PartCmd;
	cmdWrap->cmds["/BACK"]			= &ClientWindow::BackCmd;
	cmdWrap->cmds["/ABOUT"]			= &ClientWindow::AboutCmd;
	cmdWrap->cmds["/PREFERENCES"]	= &ClientWindow::PreferencesCmd;
}

ClientWindow::~ClientWindow (void)
{
	if (settings) delete settings;
	if (cmdWrap)  delete cmdWrap;
}

bool
ClientWindow::QuitRequested (void)
{
	if (settings) settings->Save();
	if (id.ICompare (serverName))
	{
		BMessage msg (M_CLIENT_SHUTDOWN), reply;

		msg.AddPointer ("client", this);
		sMsgr.SendMessage (&msg, &reply);
	}

	BMessage bmsg (M_QUIT_CLIENT);
	bmsg.AddString ("id", id.String());
	bmsg.AddInt32 ("sid", sid);
	bowser_app->PostMessage (&bmsg);

	return true;
}

void
ClientWindow::Show (void)
{
	if (!settings)
	{
		settings = new WindowSettings (
			this,
			(BString  ("client:") << id).String(),
			BOWSER_SETTINGS);

		settings->Restore();

		BWindow::Show();
		NotifyRegister();

		return;
	}

	BWindow::Show();
}

void
ClientWindow::MenusBeginning (void)
{
	if (canNotify)
	{
		BSeparatorItem *separator (new BSeparatorItem);
		BMessage *msg;

		mServer->AddItem (separator, 4);

		if ((notifyMask & NOTIFY_CONT_BIT) != 0)
		{
			msg = new BMessage (M_NOTIFICATION);
			msg->AddInt32 ("which", 0);

			mServer->AddItem (mNotification[0] = new BMenuItem (
				"Content notification", msg), 5);
			mNotification[0]->SetMarked (settings->notification[0]);
		}

		if ((notifyMask & NOTIFY_NICK_BIT) != 0)
		{
			msg = new BMessage (M_NOTIFICATION);
			msg->AddInt32 ("which", 1);
			mServer->AddItem (mNotification[1] = new BMenuItem (
				"Nick notification", msg), 6);
			mNotification[1]->SetMarked (settings->notification[1]);

			msg = new BMessage (M_NOTIFICATION);
			msg->AddInt32 ("which", 2);
			mServer->AddItem (mNotification[2] = new BMenuItem (
				"Other notification", msg), 7);
			mNotification[2]->SetMarked (settings->notification[2]);
		}
	}

	mHistory->SetEnabled (history->HasHistory());

	if (dynamic_cast<ServerWindow *>(this) == 0)
	{
		BMessage *msg (new BMessage (M_NOTIFY_SELECT));
		BMenuItem *item;

		mWindow->AddItem (item = new BMenuItem ("Server", msg, '/'), 6);
		item->SetTarget (sMsgr);
	}
}

void
ClientWindow::MenusEnded (void)
{
	if (mNotification[0] || mNotification[1] || mNotification[2])
	{
		BMenuItem *item;

		// Separator
		item = mServer->RemoveItem (4);
		delete item;

		if (mNotification[0])
		{
			item = mServer->RemoveItem (4);
			delete item;
		}

		if (mNotification[1])
		{
			item = mServer->RemoveItem (4);
			delete item;
		}

		if (mNotification[2])
		{
			item = mServer->RemoveItem (4);
			delete item;
		}

		mNotification[0] = mNotification[1] = mNotification[2] = 0;
	}

	if (dynamic_cast<ServerWindow *>(this) == 0)
	{
		BMenuItem *item (mWindow->RemoveItem (6));
		delete item;
	}
}

void
ClientWindow::DispatchMessage (BMessage *msg, BHandler *handler)
{
	if (msg->what == M_STATE_CHANGE)
		StateChange (msg);
	else
		BWindow::DispatchMessage (msg, handler);
}

void
ClientWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NOTIFY_SELECT:
		{
			if (IsHidden())
				Show();

			if (!IsActive())
				Activate (true);
			break;
		}

		// 22/8/99: this will now look for "text" to add to the
		// input view. -jamie
		case M_INPUT_FOCUS:
		{
			BString text;
			if( msg->HasString ("text") )
			{
				text = input->Text ();
				text.Append (msg->FindString("text"));
				input->SetText (text.String());
			}	
			input->MakeFocus (true);
			// We don't like your silly selecting-on-focus.
			input->TextView()->Select (input->TextView()->TextLength(),
				input->TextView()->TextLength()); 

			break;
		}


		// This could probably be used for some scripting
		// capability -- oooh, neato!
		case M_SUBMIT_RAW:
		{
			bool lines;

			msg->FindBool ("lines", &lines);

			if (lines)
			{
				BMessage *buffer (new BMessage (*msg));
				thread_id tid;

				buffer->AddPointer ("client", this);

				tid = spawn_thread (
					TimedSubmit,
					"Timed Submit",
					B_LOW_PRIORITY,
					buffer);

				resume_thread (tid);
			}
			else
			{
				BString buffer (input->Text());

				for (int32 i = 0; msg->HasString ("data", i); ++i)
				{
					const char *data;

					msg->FindString ("data", i, &data);
					buffer << (i ? " " : "") << data;
				}

				input->SetText (buffer.String());

				input->TextView()->Select (buffer.Length(), buffer.Length());
				input->TextView()->ScrollToSelection();
			}

			break;
		}

		case M_PREVIOUS_INPUT:
			history->PreviousBuffer (input);
			break;

		case M_NEXT_INPUT:
			history->NextBuffer (input);
			break;

		case M_SUBMIT:
		{
			const char *buffer;
			bool clear (true),
				history (true);
			
			msg->FindString ("input", &buffer);

			if (msg->HasBool ("clear"))
				msg->FindBool ("clear", &clear);

			if (msg->HasBool ("history"))
				msg->FindBool ("history", &history);

			Submit (buffer, clear, history);
			break;
		}

		case M_DISPLAY:
		{
			const char *buffer;

			for (int32 i = 0; msg->HasMessage ("packed", i); ++i)
			{
				BFont *font (&myFont);
				const rgb_color *color (&textColor); 
				ssize_t size (sizeof (rgb_color));
				bool timeStamp (false);
				BMessage packed;

				msg->FindMessage ("packed", i, &packed);

				if (packed.HasPointer ("font"))
					packed.FindPointer ("font", reinterpret_cast<void **>(&font));

				if (packed.HasData ("color", B_RGB_COLOR_TYPE))
					packed.FindData ("color",
						B_RGB_COLOR_TYPE,
						reinterpret_cast<const void **>(&color),
						&size);

				if (packed.HasBool ("timestamp"))
					packed.FindBool ("timestamp", &timeStamp);

				packed.FindString ("msgz", &buffer);
				Display (buffer, color, font, timeStamp);
			}
			break;
		}

		case M_NOTIFICATION:
		{
			BMenuItem *item;
			int32 which;

			msg->FindInt32 ("which", &which);

			settings->notification[which]
				= settings->notification[which] ? false : true;

			msg->FindPointer ("source", reinterpret_cast<void **>(&item));
			item->SetMarked (settings->notification[which]);
			break;
		}

		case M_CHANNEL_MSG:
		{
			const char *theNick;
			const char *theMessage;
			bool me (false), gotOther (false), gotNick (false);
			BString knownAs;

			msg->FindString("nick", &theNick);
			msg->FindString("msgz", &theMessage);

			if (FirstKnownAs (theNick, knownAs, &me) != B_ERROR && !me)
				gotOther = true;

			int32 count (nickTimes.CountItems());
			bigtime_t now (system_time());

			for (int32 i = 0; i < count; ++i)
			{
				TimedNick *tNick (reinterpret_cast<TimedNick *>(nickTimes.ItemAt
					(i)));

				if (now > tNick->when + atoi (autoNickTime.String()) * 1000000)
				{
					nickTimes.RemoveItem (tNick);
					delete tNick;
					--count;
					--i;
				}
				else if (tNick->nick.ICompare (theNick) == 0)
					gotOther = true;
			}


			if (theMessage[0] == '\1')
			{
				BString aMessage (theMessage);

				aMessage.Truncate (aMessage.Length() - 1);				
				aMessage.RemoveFirst ("\1ACTION ");
				aMessage.RemoveAll ("\1");	// JAVirc appends an illegal space at
											// the end, so .Truncate doesn't remove
											// the \1
				
				BString tempString("* ");
				tempString << theNick << " " << aMessage << '\n';
				Display (tempString.String(), &actionColor, 0, true);
			}
			else
			{
				Display ("<", 0, 0, true);

				#ifdef DEV_BUILD
				if (ShowId)
				{
					Display (id.String(), 0);
					Display (":", 0);
				}
				#endif

				Display (
					theNick,
					theNick == myNick ? &myNickColor : &nickColor);

				BString tempString;
				tempString << "> " << theMessage << "\n";

				int32 place;

				while ((place = FirstKnownAs (tempString, knownAs, &me)) !=
					B_ERROR)
				{
					BString buffer;

					if (place)
					{
						tempString.MoveInto (buffer, 0, place);
						Display (buffer.String(), 0);
					}

					tempString.MoveInto (buffer, 0, knownAs.Length());

					if (me)
					{
						Display (buffer.String(), &myNickColor);
						gotNick = true;
					}
					else
					{
						Display (buffer.String(), &nickColor);
						gotOther = true;
					}
				}

				Display (tempString.String(), 0);
			}

			if (!IsActive()
			&& ((gotNick
			&&   settings->notification[1])
			||  (gotOther
			&&   settings->notification[2])))
			{
				BMessage msg (M_NICK_CLIENT);

				msg.AddString ("id",  id.String());
				msg.AddInt32  ("sid", sid);
				msg.AddBool ("me", gotNick && settings->notification[1]);
				bowser_app->PostMessage (&msg);
			}
			break;
		}

		case M_CHANGE_NICK:
		{
			const char *oldNick;

			msg->FindString ("oldnick", &oldNick);

			if (myNick.ICompare (oldNick) == 0)
				myNick = msg->FindString ("newnick");

			BMessage display;
			if (msg->FindMessage ("display", &display) == B_NO_ERROR)
				ClientWindow::MessageReceived (&display);

			break;
		}

		case M_HISTORY_SELECT:
			
			if (history->HasHistory())
			{
				history->DoPopUp (true);
				break;
			}

		case M_SCROLL_TOGGLE:
			scrolling = scrolling ? false : true;
			mScrolling->SetMarked (scrolling);
			break;

		default:
			BWindow::MessageReceived (msg);
	}
}

void
ClientWindow::FrameResized (float width, float height)
{
	mList->Message()->ReplaceRect ("frame", Frame());
	mIgnore->Message()->ReplaceRect ("frame", Frame());
	mNotifyWindow->Message()->ReplaceRect ("frame", Frame());

	BWindow::FrameResized (width, height);
}

int32
ClientWindow::FirstKnownAs (
	const BString &data,
	BString &result,
	bool *me)
{
	int32 hit (data.Length()), place;
	BString target;

	if ((place = FirstSingleKnownAs (data, myNick)) != B_ERROR)
	{
		result = myNick;
		hit = place;
		*me = true;
	}

	for (int32 i = 1; (target = GetWord (alsoKnownAs.String(), i)) != "-9z99"; ++i)
	{
		if ((place = FirstSingleKnownAs (data, target)) != B_ERROR
		&&   place < hit)
		{
			result = target;
			hit = place;
			*me = true;
		}
	}

	for (int32 i = 1; (target = GetWord (otherNick.String(), i)) != "-9z99"; ++i)
	{
		if ((place = FirstSingleKnownAs (data, target)) != B_ERROR
		&&   place < hit)
		{
			result = target;
			hit = place;
			*me = false;
		}
	}
			
	return hit < data.Length() ? hit : B_ERROR;
}

int32
ClientWindow::FirstSingleKnownAs (const BString &data, const BString &target)
{
	int32 place;

	if ((place = data.IFindFirst (target)) != B_ERROR
	&&  (place == 0
	||   isspace (data[place - 1])
	||   ispunct (data[place - 1]))
	&&  (place + target.Length() == data.Length()
	||   isspace (data[place + target.Length()])
	||   ispunct (data[place + target.Length()])))
		return place;

	return B_ERROR;
}

void
ClientWindow::WindowActivated (bool active)
{
	BMessage msg (M_ACTIVATION_CHANGE), reply;
	BMessenger msgr (bowser_app);

	msg.AddString ("id", id.String());
	msg.AddInt32 ("sid",  sid);
	msg.AddBool ("active", active);

	msgr.SendMessage (&msg, &reply);

	if (active)
	{
		input->MakeFocus (true);
		input->TextView()->Select (
			input->TextView()->TextLength(),
			input->TextView()->TextLength());
	}
}

void
ClientWindow::Submit (
	const char *buffer,
	bool clear,
	bool historyAdd)
{
	BString cmd;
	
	if (historyAdd)
		cmd = history->Submit (buffer);
	else
		cmd = buffer;

	if (clear) input->SetText ("");
	if (cmd.Length()
	&& !SlashParser (cmd.String())
	&&   cmd[0] != '/')
		Parser (cmd.String());
}

int32
ClientWindow::TimedSubmit (void *arg)
{
	BMessage *msg (reinterpret_cast<BMessage *>(arg));
	ClientWindow *client;
	BString buffer;
	int32 i;

	msg->FindPointer ("client", reinterpret_cast<void **>(&client));

	for (i = 0; msg->HasString ("data", i); ++i)
	{
		const char *data;

		msg->FindString ("data", i, &data);

		if (client->Lock())
		{
			if (!client->SlashParser (data))
				client->Parser (data);

			client->Unlock();

			// A small attempt to appease the
			// kicker gods
			snooze (35000);
		}
	}

	delete msg;
	return 0;
}

void
ClientWindow::PackDisplay (
	BMessage *msg,
	const char *buffer,
	const rgb_color *color,
	const BFont *font,
	bool timestamp)
{
	BMessage packed;

	packed.AddString ("msgz", buffer);

	if (color)
		packed.AddData (
			"color",
			B_RGB_COLOR_TYPE,
			color,
			sizeof (color));

	if (font)
		packed.AddPointer ("font", font);

	if (timestamp)
		packed.AddBool ("timestamp", timestamp);

	msg->AddMessage ("packed", &packed);
}


void
ClientWindow::Display (
	const char *buffer,
	const rgb_color *color,
	const BFont *font,
	bool timeStamp)
{
	if (timeStamp && timeStampState)
		text->DisplayChunk (
			TimeStamp().String(),
			&textColor,
			&myFont);

	text->DisplayChunk (
		buffer,
		color ? color : &textColor,
		font  ? font  : &myFont);

	// Deskbar notification only if inactive
	if (!IsActive()
	&&  settings
	&&  settings->notification[0])
	{
		BMessage msg (M_NEWS_CLIENT);

		msg.AddString ("id",  id.String());
		msg.AddInt32  ("sid", sid);
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::Parser (const char *)
{
	// do nuttin
}

void
ClientWindow::TabExpansion (void)
{
}

void
ClientWindow::DroppedFile (BMessage *)
{
	
}

bool
ClientWindow::Scrolling (void) const
{
	BScrollBar *bar (scroll->ScrollBar (B_VERTICAL));
	float maxValue, minValue;

	bar->GetRange (&minValue, &maxValue);

	return scrolling && bar->Value() == maxValue;
}

bool
ClientWindow::SlashParser (const char *data)
{
	BString first (GetWord (data, 1));
	
	first.ToUpper();

	map<BString, CmdFunc>::iterator it;

	if ((it = cmdWrap->cmds.find (first)) != cmdWrap->cmds.end())
	{
		CmdFunc &cmd (it->second);

//		if (first != "-9z99")
			(this->*cmd) (data);
//		else
//			(this->*cmd) (void);

		return true;
	}

	return false;
}

void
ClientWindow::ChannelMessage (
	const char *msgz,
	const char *nick,
	const char *ident,
	const char *address)
{
	BMessage msg (M_CHANNEL_MSG);

	msg.AddString ("msgz", msgz);

	if (nick)    msg.AddString ("nick", nick);
	if (ident)   msg.AddString ("ident", ident);
	if (address) msg.AddString ("address", address);

	PostMessage (&msg);
}

void
ClientWindow::ActionMessage (
	const char *msgz,
	const char *nick)
{
	BMessage send (M_SERVER_SEND);

	AddSend (&send, "PRIVMSG ");
	AddSend (&send, id);
	AddSend (&send, " :\1ACTION ");
	AddSend (&send, msgz);
	AddSend (&send, "\1");
	AddSend (&send, endl);

	BString theAction ("\1ACTION ");
	theAction << msgz << "\1";

	ChannelMessage (theAction.String(), nick);
}


void
ClientWindow::JoinCmd (const char *data)
{
	BString channel (GetWord (data, 2));

	if (channel != "-9z99")
	{
		if (channel[0] != '#' && channel[0] != '&')
			channel.Prepend("#");

		BMessage send (M_SERVER_SEND);

		AddSend (&send, "JOIN ");
		AddSend (&send, channel);

		BString key (GetWord (data, 3));
		if (key != "-9z99")
		{
			AddSend (&send, " ");
			AddSend (&send, key);
		}

		AddSend (&send, endl);
	}
}

void
ClientWindow::NickCmd (const char *data)
{
	BString newNick (GetWord (data, 2));

	if (newNick != "-9z99")
	{
		BString tempString ("*** Trying new nick ");

		tempString << newNick << ".\n";
		Display (tempString.String(), 0);

		BMessage send (M_SERVER_SEND);
		AddSend (&send, "NICK ");
		AddSend (&send, newNick);
		AddSend (&send, endl);
	}
}

void
ClientWindow::MsgCmd (const char *data)
{
	BString theRest (RestOfString (data, 3));
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99"
	&&  theRest != "-9z99"
	&&  myNick.ICompare (theNick))
	{
		if (bowser_app->GetMessageOpenState())
		{
			BMessage msg (OPEN_MWINDOW);
			BMessage buffer (M_SUBMIT);

			buffer.AddString ("input", theRest.String());
			msg.AddMessage ("msg", &buffer);
			msg.AddString ("nick", theNick.String());
			sMsgr.SendMessage (&msg);
		}
		else
		{
			BString tempString;
			
			tempString << "[M]-> " << theNick << " > " << theRest << "\n";
			Display (tempString.String(), 0);

			BMessage send (M_SERVER_SEND);
			AddSend (&send, "PRIVMSG ");
			AddSend (&send, theNick);
			AddSend (&send, " :");
			AddSend (&send, theRest);
			AddSend (&send, endl);
		}

	}
}

void
ClientWindow::NickServCmd (const char *data)
{
	BString theRest (RestOfString (data, 2));

	if (theRest != "-9z99")
	{
		if (bowser_app->GetMessageOpenState())
		{
			BMessage msg (OPEN_MWINDOW);
			BMessage buffer (M_SUBMIT);

			buffer.AddString ("input", theRest.String());
			msg.AddMessage ("msg", &buffer);
			msg.AddString ("nick", "NickServ");
			sMsgr.SendMessage (&msg);
		}
		else
		{
			BString tempString;
			
			tempString << "[M]-> NickServ > " << theRest << "\n";
			Display (tempString.String(), 0);

			BMessage send (M_SERVER_SEND);
			AddSend (&send, "PRIVMSG NickServ");
			AddSend (&send, " :");
			AddSend (&send, theRest);
			AddSend (&send, endl);
		}

	}
}

void
ClientWindow::ChanServCmd (const char *data)
{
	BString theRest (RestOfString (data, 2));

	if (theRest != "-9z99")
	{
		if (bowser_app->GetMessageOpenState())
		{
			BMessage msg (OPEN_MWINDOW);
			BMessage buffer (M_SUBMIT);

			buffer.AddString ("input", theRest.String());
			msg.AddMessage ("msg", &buffer);
			msg.AddString ("nick", "ChanServ");
			sMsgr.SendMessage (&msg);
		}
		else
		{
			BString tempString;
			
			tempString << "[M]-> ChanServ > " << theRest << "\n";
			Display (tempString.String(), 0);

			BMessage send (M_SERVER_SEND);
			AddSend (&send, "PRIVMSG ChanServ");
			AddSend (&send, " :");
			AddSend (&send, theRest);
			AddSend (&send, endl);
		}

	}
}

void
ClientWindow::MemoServCmd (const char *data)
{
	BString theRest (RestOfString (data, 2));

	if (theRest != "-9z99")
	{
		if (bowser_app->GetMessageOpenState())
		{
			BMessage msg (OPEN_MWINDOW);
			BMessage buffer (M_SUBMIT);

			buffer.AddString ("input", theRest.String());
			msg.AddMessage ("msg", &buffer);
			msg.AddString ("nick", "MemoServ");
			sMsgr.SendMessage (&msg);
		}
		else
		{
			BString tempString;
			
			tempString << "[M]-> MemoServ > " << theRest << "\n";
			Display (tempString.String(), 0);

			BMessage send (M_SERVER_SEND);
			AddSend (&send, "PRIVMSG MemoServ");
			AddSend (&send, " :");
			AddSend (&send, theRest);
			AddSend (&send, endl);
		}

	}
}

void
ClientWindow::UptimeCmd (const char *data)
{
	BString parms (GetWord(data, 2));
	
	if ((id != serverName) && (parms == "-9z99"))
	{
		BString uptime (DurationString(system_time()));
		BString expandedString;
		
		const char *expansions[1];
		expansions[0] = uptime.String();
		expandedString = ExpandKeyed (bowser_app->GetCommand (CMD_UPTIME).String(), "U",
			expansions);
		expandedString.RemoveFirst("\n");
		
		BMessage send (M_SERVER_SEND);
		AddSend (&send, "PRIVMSG ");
		AddSend (&send, id);
		AddSend (&send, " :");
		AddSend (&send, expandedString.String());
		AddSend (&send, endl);
		
		ChannelMessage (expandedString.String(), myNick.String());
	}
	else if ((parms == "-l") || (id == serverName)) // echo locally
	{
		//BString uptime (UptimeString());
		BString uptime (DurationString(system_time()));
		BString expandedString;
		
		const char *expansions[1];
		expansions[0] = uptime.String();
		expandedString = ExpandKeyed (bowser_app->GetCommand (CMD_UPTIME).String(), "U",
			expansions);
		expandedString.RemoveFirst("\n");
		
		BString tempString;
			
		tempString << "Uptime: " << expandedString << "\n";
		Display (tempString.String(), &whoisColor);
		
	}
		
}

void
ClientWindow::DnsCmd (const char *data)
{
	BString parms (GetWord(data, 2));
	ChannelWindow *window;
	if ((window = dynamic_cast<ChannelWindow *>(this)))
	{
			int32 count (window->namesList->CountItems());
			
			for (int32 i = 0; i < count; ++i)
			{
				NameItem *item ((NameItem *)(window->namesList->ItemAt (i)));
				
				if (!item->Name().ICompare (parms.String(), strlen (parms.String()))) //nick
				{
					BMessage send (M_SERVER_SEND);
					AddSend (&send, "USERHOST ");
					AddSend (&send, item->Name().String());
					AddSend (&send, endl);
					PostMessage(&send);	
					return;				
				}
			}
	}
		
	if (parms != "-9z99")
	{
		BMessage *msg (new BMessage);
		msg->AddString ("lookup", parms.String());
		msg->AddPointer ("client", this);
		
		thread_id lookupThread = spawn_thread (
			DNSLookup,
			"dns_lookup",
			B_NORMAL_PRIORITY,
			msg);

		resume_thread (lookupThread);
	}

}

void
ClientWindow::CtcpCmd (const char *data)
{
	BString theTarget (GetWord (data, 2));
	BString theAction (RestOfString (data, 3));

	if (theAction != "-9z99")
	{
		theAction.ToUpper();

		if (theAction.ICompare ("PING") == 0)
		{
			time_t now (time (0));

			theAction << " " << now;
		}

		CTCPAction (theTarget, theAction);
		
//		BString tempString ("[C]-> ");
//		tempString << theTarget << " -> " << theAction << '\n';
//		Display (tempString.String(), 0);

		BMessage send (M_SERVER_SEND);
		AddSend (&send, "PRIVMSG ");
		AddSend (&send, theTarget << " :\1" << theAction << "\1");
		AddSend (&send, endl);
	}
}

void
ClientWindow::CTCPAction(BString theTarget, BString theMsg)
{
	BString theCTCP = GetWord(theMsg.String(), 1).ToUpper();
	BString theRest = RestOfString(theMsg.String(), 2);
	
	BString tempString ("[CTCP->");
	
	tempString << theTarget << "] " << theCTCP;
	
	if (theRest != "-9z99")
	{
		tempString << " " << theRest << '\n';
	}
	else
	{
		tempString << '\n';
	}
	
	Display (tempString.String(), &ctcpReqColor, &serverFont);
	
}

void
ClientWindow::MeCmd (const char *data)
{
	BString theAction (RestOfString (data, 2));

	if (theAction != "-9z99")
		ActionMessage (theAction.String(), myNick.String());
}

void
ClientWindow::DescribeCmd (const char *data)
{
    BString theTarget (GetWord (data, 2));
	BString theAction (RestOfString (data, 3));
	
	if (theAction != "-9z99") {
	
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "PRIVMSG ");
		AddSend (&send, theTarget);
		AddSend (&send, " :\1ACTION ");
		AddSend (&send, theAction);
		AddSend (&send, "\1");
		AddSend (&send, endl);
	
		BString theActionMessage ("[ACTION]-> ");
		theActionMessage << theTarget << " -> " << theAction << "\n";

		Display (theActionMessage.String(), 0);
	}

}

void
ClientWindow::QuitCmd (const char *data)
{
	BString theRest (RestOfString (data, 2));
	BString buffer;

	if (theRest == "-9z99")
	{
		const char *expansions[1];
		BString version (VERSION);

		expansions[0] = version.String();
		theRest = ExpandKeyed (bowser_app
			->GetCommand (CMD_QUIT).String(), "V", expansions);
	}

	buffer << "QUIT :" << theRest;

	BMessage msg (B_QUIT_REQUESTED);
	msg.AddString ("bowser:quit", buffer.String());
	sMsgr.SendMessage (&msg);
}

void
ClientWindow::KickCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		BString theReason (RestOfString (data, 3));

		if (theReason == "-9z99")
		{
			// No expansions
			theReason = bowser_app
				->GetCommand (CMD_KICK);
		}

		BMessage send (M_SERVER_SEND);

		AddSend (&send, "KICK ");
		AddSend (&send, id);
		AddSend (&send, " ");
		AddSend (&send, theNick);
		AddSend (&send, " :");
		AddSend (&send, theReason);
		AddSend (&send, endl);
	}
}
	
void
ClientWindow::WhoIsCmd (const char *data)
{
	BString theNick (GetWord (data, 2));
	BString theNick2 (GetWord (data, 3));

	if (theNick != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "WHOIS ");
		AddSend (&send, theNick);


		if (theNick2 != "-9z99")
		{
			AddSend (&send, " ");
			AddSend (&send, theNick2);
		}

		AddSend (&send, endl);
	}
}

void
ClientWindow::UserhostCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "USERHOST ");
		AddSend (&send, theNick);
		AddSend (&send, endl);
	}
}

void
ClientWindow::PartCmd (const char *data)
{
	BMessage msg (B_QUIT_REQUESTED);

	msg.AddBool ("bowser:part", true);
	PostMessage (&msg);
}


void
ClientWindow::OpCmd (const char *data)
{
	BString theNick (RestOfString (data, 2));

	if (theNick != "-9z99")
	{
		// TODO only applies to a channel

		BMessage send (M_SERVER_SEND);

		AddSend (&send, "MODE ");
		AddSend (&send, id);
		AddSend (&send, " +oooo ");
		AddSend (&send, theNick);
		AddSend (&send, endl);
	}
}

void
ClientWindow::DopCmd (const char *data)
{
	BString theNick (RestOfString (data, 2));

	if (theNick != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "MODE ");
		AddSend (&send, id);
		AddSend (&send, " -oooo ");
		AddSend (&send, theNick);
		AddSend (&send, endl);
	}
}

void
ClientWindow::ModeCmd (const char *data)
{
	BString theMode (RestOfString (data, 3));
	BString theTarget (GetWord (data, 2));

	if (theTarget != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "MODE ");

		if (theMode == "-9z99")
			AddSend (&send, theTarget);
		else
			AddSend (&send, theTarget << " " << theMode);

		AddSend (&send, endl);
	}
}

void
ClientWindow::Mode2Cmd (const char *data)
{
	BString theMode (RestOfString (data, 2));
	
	BMessage send (M_SERVER_SEND);
	AddSend (&send, "MODE ");

	if (id == serverName)
		AddSend (&send, myNick);
	else if (id[0] == '#' || id[0] == '&')
		AddSend (&send, id);
	else
		AddSend (&send, myNick);
	 
	if (theMode != "-9z99")
	{
			AddSend (&send, " ");
			AddSend (&send, theMode);
	}

	AddSend (&send, endl);
}
	
void
ClientWindow::PingCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		long theTime (time (0));
		BString tempString ("/CTCP ");

		tempString << theNick << " PING " << theTime;
		SlashParser (tempString.String());
	}
}

void
ClientWindow::VersionCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		BString tempString ("/CTCP ");

		tempString << theNick << " VERSION";
		SlashParser (tempString.String());
	}
	else
	{
	  	BMessage send (M_SERVER_SEND);

		AddSend (&send, "VERSION");
		AddSend (&send, endl);
	}
		
}

void
ClientWindow::NoticeCmd (const char *data)
{
	BString theTarget (GetWord (data, 2));
	BString theMsg (RestOfString (data, 3));

	if (theMsg != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "NOTICE ");
		AddSend (&send, theTarget);
		AddSend (&send, " :");
		AddSend (&send, theMsg);
		AddSend (&send, endl);

		BString tempString ("[N]-> ");
		tempString << theTarget << " -> " << theMsg << '\n';

		Display (tempString.String(), 0);
	}
}

void
ClientWindow::RawCmd (const char *data)
{
	BString theRaw (RestOfString (data, 2));

	if (theRaw != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, theRaw);
		AddSend (&send, endl);

		BString tempString ("[R]-> ");
		tempString << theRaw << '\n';

		Display (tempString.String(), 0);

	}
}

void
ClientWindow::StatsCmd (const char *data)
{
	BString theStat (RestOfString (data, 2));

	if (theStat != "-9z99")
	{
		BMessage send (M_SERVER_SEND);
		
		AddSend (&send, "STATS ");
		AddSend (&send, theStat);
		AddSend (&send, endl);
	}
}


void
ClientWindow::AwayCmd (const char *data)
{
	BString theReason (RestOfString (data, 2));
	BString tempString;


	if (theReason != "-9z99")
	{
		//nothing to do
	}
	else
	{
		theReason = "BRB"; // Todo: make a default away msg option
	}

	const char *expansions[1];
	expansions[0] = theReason.String();
	
	tempString = ExpandKeyed (bowser_app->GetCommand (CMD_AWAY).String(), "R",
		expansions);
	tempString.RemoveFirst("\n");

	BMessage send (M_SERVER_SEND);
	AddSend (&send, "AWAY");
	AddSend (&send, " :");
	AddSend (&send, theReason.String());
	AddSend (&send, endl);
	
	if (id != serverName)
	{
		ActionMessage (tempString.String(), myNick.String());
	}
}

void
ClientWindow::BackCmd (const char *data)
{
	BMessage send (M_SERVER_SEND);

	AddSend (&send, "AWAY");
	AddSend (&send, endl);

	if (id != serverName)
	{
		ActionMessage (
			bowser_app->GetCommand (CMD_BACK).String(),
			myNick.String());
	}
}

void
ClientWindow::KillCmd (const char *data)
{
	BString theNick (GetWord (data, 2));
	BString theKill (RestOfString (data, 3));

	if (theNick != "-9z99")
	{
		BMessage send (M_SERVER_SEND);
		
		// nick
		AddSend (&send, "KILL ");
		AddSend (&send, theNick);
		
		if (theKill != "-9z99")
		{
			// reason
			AddSend (&send, " :");
			AddSend (&send, theKill);
		}
		
		AddSend (&send, endl);
	}
}

void
ClientWindow::WallopsCmd (const char *data)
{
	BString theWallops (RestOfString (data, 2));

	if (theWallops != "-9z99")
	{
		BMessage send (M_SERVER_SEND);
		
		AddSend (&send, "WALLOPS :");
		AddSend (&send, theWallops);
		AddSend (&send, endl);
	}
}

void
ClientWindow::OperCmd (const char *data)
{
	BString theOper (RestOfString (data, 2));

	if (theOper != "-9z99")
	{
		BMessage send (M_SERVER_SEND);
		
		AddSend (&send, "OPER ");
		AddSend (&send, theOper);
		AddSend (&send, endl);
	}
}

void
ClientWindow::AdminCmd (const char *data)
{
	BString theAdmin (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);
	
	AddSend (&send, "ADMIN");
	
	if (theAdmin != "-9z99")
	{
		AddSend (&send, " ");
		AddSend (&send, theAdmin);
	}
	AddSend (&send, endl);
}


void
ClientWindow::RehashCmd (const char *data)
{
	BString theRehash (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);
	
	AddSend (&send, "REHASH");
	
	if (theRehash != "-9z99")
	{
		AddSend (&send, " ");
		AddSend (&send, theRehash);
	}
	AddSend (&send, endl);
}

void
ClientWindow::TraceCmd (const char *data)
{
	BString theTrace (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);
	
	AddSend (&send, "TRACE");
	
	if (theTrace != "-9z99")
	{
		AddSend (&send, " ");
		AddSend (&send, theTrace);
	}
	AddSend (&send, endl);
}

void
ClientWindow::InfoCmd (const char *data)
{
	BString theInfo (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);
	
	AddSend (&send, "INFO");
	
	if (theInfo != "-9z99")
	{
		AddSend (&send, " ");
		AddSend (&send, theInfo);
	}
	AddSend (&send, endl);
}

void
ClientWindow::MotdCmd (const char *data)
{
	BString theMotd (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);
	
	AddSend (&send, "MOTD");
	
	if (theMotd != "-9z99")
	{
		AddSend (&send, " ");
		AddSend (&send, theMotd);
	}
	AddSend (&send, endl);
}

void
ClientWindow::TopicCmd (const char *data)
{
	BString theChan (id);
	BString theTopic (RestOfString (data, 2));
	BMessage send (M_SERVER_SEND);

	AddSend (&send, "TOPIC ");

	if (theTopic == "-9z99")
		AddSend (&send, theChan);
	else
		AddSend (&send, theChan << " :" << theTopic);
	AddSend (&send, endl);
}

void
ClientWindow::NamesCmd (const char *data)
{
	BString theChan (GetWord (data, 2));

	if (theChan != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "NAMES ");
		AddSend (&send, theChan);
		AddSend (&send, endl);
	}
}

void
ClientWindow::QueryCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		BMessage msg (OPEN_MWINDOW);

		msg.AddString ("nick", theNick.String());
		sMsgr.SendMessage (&msg);
	}
}

void
ClientWindow::WhoWasCmd (const char *data)
{
	BString theNick (GetWord (data, 2));

	if (theNick != "-9z99")
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "WHOWAS ");
		AddSend (&send, theNick);
		AddSend (&send, endl);
	}
}

void
ClientWindow::DccCmd (const char *data)
{
	BString secondWord (GetWord (data, 2));
	BString theNick (GetWord (data, 3));
	BString theFile (RestOfString(data, 4));
	
	if (secondWord.ICompare ("SEND") == 0
	&&  theNick != "-9z99")
	{
		BMessage *msg (new BMessage (CHOSE_FILE));
		msg->AddString ("nick", theNick.String());
		if (theFile != "-9z99")
		{	
			char filePath[B_PATH_NAME_LENGTH] = "\0";
			if (theFile.ByteAt(0) != '/')
			{
				find_directory(B_USER_DIRECTORY, 0, false, filePath, B_PATH_NAME_LENGTH);
				filePath[strlen(filePath)] = '/';
			}
			strcat(filePath, theFile.LockBuffer(0));
			theFile.UnlockBuffer();

			// use BPath to resolve relative pathnames, above code forces it
			// to use /boot/home as a working dir as opposed to the app path

			BPath sendPath(filePath, NULL, true);
			
			// the BFile is used to verify if the file exists
			// based off the documentation get_ref_for_path *should*
			// return something other than B_OK if the file doesn't exist
			// but that doesn't seem to be working correctly
			
			BFile sendFile(sendPath.Path(), B_READ_ONLY);
			
			// if the file exists, send, otherwise drop to the file panel
			
			if (sendFile.InitCheck() == B_OK)
			{
				entry_ref ref;
				get_ref_for_path(sendPath.Path(), &ref);
				msg->AddRef("refs", &ref);
				be_app_messenger.SendMessage(msg);	
				return;	
			}
		}
		BFilePanel *myPanel (new BFilePanel);
		BString myTitle ("Sending a file to ");

		myTitle.Append (theNick);
		myPanel->Window()->SetTitle (myTitle.String());

		myPanel->SetMessage (msg);

		myPanel->SetButtonLabel (B_DEFAULT_BUTTON, "Send");
		myPanel->SetTarget (sMsgr);
		myPanel->Show();
	}
	else if (secondWord.ICompare ("CHAT") == 0
	&&       theNick != "-9z99")
	{
		BMessage msg (CHAT_ACTION);

		msg.AddString ("nick", theNick.String());

		sMsgr.SendMessage (&msg);
	}
}

void
ClientWindow::ClearCmd (const char *data)
{
	text->ClearView();
}

void
ClientWindow::InviteCmd (const char *data)
{
	BString theUser (GetWord (data, 2));

	if (theUser != "-9z99")
	{
		BString theChan (GetWord (data, 3));

		if (theChan == "-9z99")
			theChan = id;

		BMessage send (M_SERVER_SEND);

		AddSend (&send, "INVITE ");
		AddSend (&send, theUser << " " << theChan);
		AddSend (&send, endl);
	}
}

void
ClientWindow::WhoCmd (const char *data)
{
	BString theMask (GetWord (data, 2));

	BMessage send (M_SERVER_SEND);

	AddSend (&send, "WHO ");
	AddSend (&send, theMask);
	AddSend (&send, endl);
}

void
ClientWindow::ListCmd (const char *data)
{
	BMessage msg (M_LIST_COMMAND);

	msg.AddString ("cmd", data);
	msg.AddString ("server", serverName.String());
	msg.AddRect ("frame", Frame());
	bowser_app->PostMessage (&msg);
}

void
ClientWindow::IgnoreCmd (const char *data)
{
	BString rest (RestOfString (data, 2));

	if (rest != "-9z99")
	{
		BMessage msg (M_IGNORE_COMMAND);

		msg.AddString ("cmd", rest.String());
		msg.AddString ("server", serverName.String());
		msg.AddRect ("frame", Frame());
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::UnignoreCmd (const char *data)
{
	BString rest (RestOfString (data, 2));

	if (rest != "-9z99")
	{
		BMessage msg (M_UNIGNORE_COMMAND);

		msg.AddString ("cmd", rest.String());
		msg.AddString ("server", serverName.String());
		msg.AddRect ("frame", Frame());
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::ExcludeCmd (const char *data)
{
	BString second (GetWord (data, 2)),
		rest (RestOfString (data, 3));

	if (rest != "-9z99" && rest != "-9z99")
	{
		BMessage msg (M_EXCLUDE_COMMAND);

		msg.AddString ("second", second.String());
		msg.AddString ("cmd", rest.String());
		msg.AddString ("server", serverName.String());
		msg.AddRect ("frame", Frame());
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::NotifyCmd (const char *data)
{
	BString rest (RestOfString (data, 2));

	if (rest != "-9z99")
	{
		BMessage msg (M_NOTIFY_COMMAND);

		msg.AddString ("cmd", rest.String());
		msg.AddBool ("add", true);
		msg.AddString ("server", serverName.String());
		msg.AddRect ("frame", Frame());
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::UnnotifyCmd (const char *data)
{
	BString rest (RestOfString (data, 2));

	if (rest != "-9z99")
	{
		BMessage msg (M_NOTIFY_COMMAND);

		msg.AddString ("cmd", rest.String());
		msg.AddBool ("add", false);
		msg.AddRect ("frame", Frame());
		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);
	}
}

void
ClientWindow::PreferencesCmd (const char *data)
{
	be_app_messenger.SendMessage (M_PREFS_BUTTON);
}

void
ClientWindow::AboutCmd (const char *data)
{
	be_app_messenger.SendMessage (B_ABOUT_REQUESTED);
}

void
ClientWindow::VisitCmd (const char *data)
{
	BString buffer (data);
	int32 place;

	if ((place = buffer.FindFirst (" ")) >= 0)
	{
		buffer.Remove (0, place + 1);

		const char *arguments[] = {buffer.String(), 0};
		
		

		be_roster->Launch (
			"text/html",
			1,
			const_cast<char **>(arguments));
	}
}

void
ClientWindow::SleepCmd (const char *data)
{
	BString rest (RestOfString (data, 2));

	if (rest != "-9z99")
	{
		// this basically locks up the window its run from,
		// but I can't think of a better way with our current
		// commands implementation
		int32 time = atoi(rest.String());
		snooze(time * 1000 * 100); // deciseconds? 10 = one second
	}
}


const BString &
ClientWindow::Id (void) const
{
	return id;
}

void
ClientWindow::AddSend (BMessage *msg, const char *buffer)
{
	if (strcmp (buffer, endl) == 0)
	{
		BLooper *looper;
		
		if (sMsgr.Target (&looper) == this
		||  looper == this)
			this->MessageReceived (msg);
		else
			sMsgr.SendMessage (msg);
		msg->MakeEmpty();
	}
	else
		msg->AddString ("data", buffer);
}

void
ClientWindow::AddSend (BMessage *msg, const BString &buffer)
{
	AddSend (msg, buffer.String());
}

void
ClientWindow::AddSend (BMessage *msg, int32 value)
{
	BString buffer;

	buffer << value;
	AddSend (msg, buffer.String());
}

void
ClientWindow::StateChange (BMessage *msg)
{
	if (msg->HasData ("color", B_RGB_COLOR_TYPE))
	{
		const rgb_color *color;
		int32 which;
		ssize_t size;

		msg->FindInt32 ("which", &which);
		msg->FindData (
			"color",
			B_RGB_COLOR_TYPE,
			reinterpret_cast<const void **>(&color),
			&size);

		switch (which)
		{
			case C_TEXT:
				textColor = *color;
				break;

			case C_NICK:
				nickColor = *color;
				break;

			case C_MYNICK:
				myNickColor = *color;
				break;

			case C_ACTION:
				actionColor = *color;
				break;

			case C_OP:
				opColor = *color;
				break;

			case C_URL:
				text->SetColor (which, *color);
				break;

			case C_BACKGROUND:
				text->SetViewColor (*color);
				text->Invalidate();
				break;
		}
	}

	if (msg->HasBool ("timestamp"))
		msg->FindBool ("timestamp", &timeStampState);

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

	if (msg->HasPointer ("font"))
	{
		BFont *font;
		int32 which;

		msg->FindInt32 ("which", &which);
		msg->FindPointer ("font", reinterpret_cast<void **>(&font));

		if (which == F_TEXT)
			myFont = *font;

		if (which == F_SERVER)
			serverFont = *font;

		if (which == F_INPUT)
		{
			inputFont = *font;
			input->TextView()->SetFontAndColor (&inputFont);
			input->ResizeToPreferred();
			input->MoveTo (
				2,
				status->Frame().top - input->Frame().Height() - 1);

			history->ResizeTo (
				history->Frame().Width(),
				input->Frame().Height() - 1);
			history->MoveTo (
				history->Frame().left,
				input->Frame().top);

			scroll->ResizeTo (
				scroll->Frame().Width(),
				input->Frame().top - 1);
		}

		if (which == F_URL)
			text->SetFont (which, font);
	}

	if (msg->HasString ("aka"))
	{
		const char *buffer;

		msg->FindString ("aka", &buffer);
		alsoKnownAs = buffer;
	}

	if (msg->HasString ("other nick"))
	{
		const char *buffer;

		msg->FindString ("other nick", &buffer);
		otherNick = buffer;
	}

	if (msg->HasString ("auto nick"))
	{
		const char *buffer;

		msg->FindString ("auto nick", &buffer);
		autoNickTime = buffer;
	}

	if (msg->HasBool ("cannotify"))
	{
		bool wasCanNotify (canNotify);

		msg->FindBool ("cannotify", &canNotify);
		msg->FindInt32 ("notifymask", &notifyMask);

		// If we couldn't and we can now..
		// tell the icon about us
		if (!wasCanNotify && canNotify)
			NotifyRegister();
	}
}


void
ClientWindow::NotifyRegister (void)
{
	BMessage msg (M_NEW_CLIENT);

	msg.AddString ("id", id.String());
	msg.AddString ("server", serverName.String());
	msg.AddInt32 ("sid", sid);
	msg.AddMessenger ("msgr", BMessenger (this));

	bowser_app->PostMessage (&msg);
}

float
ClientWindow::ScrollPos (void)
{
	return scroll->ScrollBar (B_VERTICAL)->Value();
}

void
ClientWindow::SetScrollPos (float value)
{
	scroll->ScrollBar (B_VERTICAL)->SetValue(value);
}

void
ClientWindow::ScrollRange (float *min, float *max)
{
	scroll->ScrollBar(B_VERTICAL)->GetRange (min, max);
}

BString
ClientWindow::DurationString (int64 value)
{
	printf ("value: %Ld\n",value);
	BString duration;
	bigtime_t micro = value;
	bigtime_t milli = micro/1000;
	bigtime_t sec = milli/1000;
	bigtime_t min = sec/60;
	bigtime_t hours = min/60;
	bigtime_t days = hours/24;

	char message[512] = "";
	if (days)
		sprintf(message, "%Ldday%s ",days,days!=1?"s":"");
	
	if (hours%24)
		sprintf(message, "%s%Ldhr%s ",message, hours%24,(hours%24)!=1?"s":"");
	
	if (min%60)
		sprintf(message, "%s%Ldmin%s ",message, min%60, (min%60)!=1?"s":"");

	sprintf(message, "%s%Ldsec%s",message, sec%60,(sec%60)!=1?"s":"");

	duration << message;

	return duration;
}

int32
ClientWindow::DNSLookup (void *arg)
{
	BMessage *msg (reinterpret_cast<BMessage *>(arg));
	const char *lookup;
	ClientWindow *client;
	
	msg->FindString ("lookup", &lookup);
	msg->FindPointer ("client", reinterpret_cast<void **>(&client));
	
	delete msg;
	
	BString resolve (lookup),
			output ("[x] ");
	
	if(isalpha(resolve[0]))
	{
		hostent *hp = gethostbyname (resolve.String());
				
		if(hp)
		{
			// ip address is in hp->h_addr_list[0];
			char addr_buf[16];
					
			in_addr *addr = (in_addr *)hp->h_addr_list[0];
			strcpy(addr_buf, inet_ntoa(*addr));

			output << "Resolved " << resolve.String() << " to " << addr_buf;
		}
		else
		{
			output << "Unable to resolve " << resolve.String();
		}
	}
	else
	{
		ulong addr = inet_addr (resolve.String());
				
		hostent *hp = gethostbyaddr ((const char *)&addr, 4, AF_INET);
		if(hp)
		{
			output << "Resolved " << resolve.String() << " to " << hp->h_name;
		}
		else
		{
			output << "Unable to resolve " << resolve.String();
		}
	}
	output << "\n";
	
	BMessage dnsMsg (M_DISPLAY);
	client->PackDisplay (&dnsMsg, output.String(), &(client->whoisColor));
	client->PostMessage(&dnsMsg);	
	
	return 0;

}

