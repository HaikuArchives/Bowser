#include <FilePanel.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <ScrollView.h>
#include <View.h>
#include <Path.h>
#include <Mime.h>
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
	  settings (settings_)
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
	  settings (settings_)
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

	mWindow->AddSeparatorItem();
	msg = new BMessage (M_SCROLL_TOGGLE);
	mWindow->AddItem (mScrolling = new BMenuItem ("Auto Scrolling", msg, 'S'));
	mScrolling->SetMarked (true);
	menubar->AddItem (mWindow);

	mHelp = new BMenu ("Help");
	mHelp->AddItem (item = new BMenuItem ("About Bowser",
		new BMessage (B_ABOUT_REQUESTED), 'A', B_SHIFT_KEY));
	item->SetTarget (bowser_app);
	mHelp->AddSeparatorItem();
	mHelp->AddItem (item = new BMenuItem ("Bugs",
		new BMessage (M_BOWSER_BUGS)));	
	
	menubar->AddItem (mHelp);
	AddChild (menubar);
	

	// A client window maintains it's own state, so we don't
	// have to lock the app everytime we want to get this information.
	// When it changes, the application looper will let us know
	// through a M_STATE_CHANGE message
	textColor		= bowser_app->GetColor (C_TEXT);
	nickColor		= bowser_app->GetColor (C_NICK);
	ctcpReqColor	= bowser_app->GetColor (C_CTCP_REQ);
	quitColor		= bowser_app->GetColor (C_QUIT);
	errorColor		= bowser_app->GetColor (C_ERROR);
	whoisColor		= bowser_app->GetColor (C_WHOIS);
	joinColor		= bowser_app->GetColor (C_JOIN);
	myNickColor		= bowser_app->GetColor (C_MYNICK);
	actionColor		= bowser_app->GetColor (C_ACTION);
	opColor			= bowser_app->GetColor (C_OP);
	inputColor		= bowser_app->GetColor (C_INPUT);
	
	myFont			= *(bowser_app->GetClientFont (F_TEXT));
	serverFont		= *(bowser_app->GetClientFont (F_SERVER));
	inputFont   	= *(bowser_app->GetClientFont (F_INPUT));
	canNotify		= bowser_app->CanNotify();
	alsoKnownAs		= bowser_app->GetAlsoKnownAs();
	otherNick		= bowser_app->GetOtherNick();
	autoNickTime	= bowser_app->GetAutoNickTime();
	notifyMask		= bowser_app->GetNotificationMask();

	
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

	input = new BTextControl (
		BRect (2, frame.top, frame.right - 15, frame.bottom),
		"Input", 0, 0,
		0,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	input->TextView()->SetFontAndColor (&inputFont, B_FONT_ALL, 
			&inputColor);
	input->SetDivider (0); 
	input->ResizeToPreferred(); 
	
	input->MoveTo ( 
		2, 
		status->Frame().top - input->Frame().Height() - 1);

	input->TextView()->AddFilter (new ClientInputFilter (this));
	
	bgView->AddChild (input);
	
	input->TextView()->SetViewColor (bowser_app->GetColor (C_INPUT_BACKGROUND));
	input->Invalidate();

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

	NotifyRegister();

	isLogging		= bowser_app->GetMasterLogState();
	scrolling		= true;
	
	SetupLogging();

}

ClientWindow::~ClientWindow (void)
{
	if (settings) delete settings;
	
	isLogging = false;
	SetupLogging();
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
	printf ("ClientWindow::Show\n");
	if (!settings)
	{
		settings = new WindowSettings (
			this,
			(BString  ("client:") << id).String(),
			BOWSER_SETTINGS);

		settings->Restore();

		BWindow::Show();

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
/*	if (msg->what == M_STATE_CHANGE)
	{
		StateChange (msg);
		return;
	}
*/	
	if (msg->what == B_KEY_DOWN)
	{
		BRect bounds = input->TextView()->Bounds();
		BFont current;
		input->TextView()->GetFontAndColor(0, &current);
		
		bounds.left = input->TextView()->LineWidth(0);
		bounds.right = bounds.left + 2 * current.Size();
		if (bounds.left > 0) bounds.left -= current.Size();
		input->TextView()->Invalidate(bounds);
	}
	
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
				printf ("1111111111in the else zone11111111\n");
				BString buffer;
				for (int32 i = 0; msg->HasString ("data", i); ++i)
				{
					const char *data;

					msg->FindString ("data", i, &data);
					buffer << (i ? " " : "") << data;
				}
				int32 start, finish;
				if (msg->FindInt32 ("selstart", &start) == B_OK)
				{
					printf ("whatever this means\n");
					
					msg->FindInt32 ("selend", &finish);
					printf ("start: %ld end: %ld\n", start, finish);
					if (start != finish)
					{
						printf ("nuking something\n");
						input->TextView()->Delete (start, finish);
					}
					
					
					if ((start == 0) && (finish == 0))
					{
						input->TextView()->Insert (input->TextView()->TextLength(), buffer.String(), buffer.Length());
						input->TextView()->Select (input->TextView()->TextLength(),
							input->TextView()->TextLength());
					}
					else
					{				
						input->TextView()->Insert (start, buffer.String(), buffer.Length());
						input->TextView()->Select (start + buffer.Length(), start + buffer.Length()); 				
					}
				}
				else
				{
					printf ("am i even being called\n");
					input->TextView()->Insert (buffer.String());
					input->TextView()->Select (input->TextView()->TextLength(),
						input->TextView()->TextLength());
				}
				
				
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
		{	
			if (history->HasHistory())
			{
				history->DoPopUp (true);
				break;
			}
		}
		
		case M_SCROLL_TOGGLE:
		{
			scrolling = scrolling ? false : true;
			mScrolling->SetMarked (scrolling);
			break;
		}

		case M_BOWSER_BUGS:
		{
			const char *arguments[] = {"http://sourceforge.net/bugs/?group_id=695", 0};
		
			be_roster->Launch (
				"text/html",
				1,
				const_cast<char **>(arguments));

			break;
		}
		
		case M_DCC_COMPLETE:
		{
			if (IsActive() || dynamic_cast<ServerWindow *>(this) != NULL)
			{
				/// set up ///
				BString nick,
				        file,
				        size,
			  	        type,
				        message ("[@] "),
				        fAck;
				int32 rate,
				      xfersize;
				bool completed (true);
			
				msg->FindString ("nick", &nick);
				msg->FindString ("file", &file);
				msg->FindString ("size", &size);
				msg->FindString ("type", &type);
				msg->FindInt32 ("transferred", &xfersize);
				msg->FindInt32 ("transferRate", &rate);
				
				BPath pFile (file.String());

				fAck << xfersize;
								
				if (size.ICompare (fAck))
					completed = false;
				
				
				/// send mesage ///				
				if (completed)
					message << "Completed ";
				else message << "Terminated ";
				
				if (type == "SEND")
					message << "send of " << pFile.Leaf() << " to ";
				else message << "receive of " << pFile.Leaf() << " from ";
				
				message << nick << " (";
				
				if (!completed)
					message << fAck << "/";
				
				message << size << " bytes), ";
				message	<< rate << " cps\n";
					
				const rgb_color disp = bowser_app->GetColor(C_CTCP_RPY);
				Display (message.String(), &disp); 
			}
			break;
		}
		
		case M_STATE_CHANGE:
		{
			StateChange (msg);
			break;
		}
		
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
		BString newTitle (Title());
		if (newTitle.ByteAt(0) == '*')
		{
			newTitle.Remove(0,1);
			SetTitle(newTitle.String());
		}
		
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
			snooze (50000);
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

	if (isLogging)
	{
		BString printbuf;
		if (timeStamp)
		{
			printbuf << TimeStamp().String();
		}
		
		printbuf << buffer;
		
		off_t len = strlen (printbuf.String());
		logFile.Write (printbuf.String(), len);
	}

	if (timeStamp && timeStampState)
		text->DisplayChunk (
			TimeStamp().String(),
			&textColor,
			&myFont);

	text->DisplayChunk (
		buffer,
		color ? color : &textColor,
		font  ? font  : &myFont);
	
	if (!IsActive())
	{
		BString newTitle (Title ());
		if (newTitle.ByteAt (0) != '*')
			newTitle.Prepend ("*");
		SetTitle (newTitle.String());
	}
	
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
	BString first (GetWord (data, 1).ToUpper());
	
	if (ParseCmd (data))
		return true;

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
	printf ("ClientWindow::StateChange %s\n", id.String());
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
			
			case C_INPUT_BACKGROUND:
				input->TextView()->SetViewColor(*color);
				input->TextView()->Invalidate();
				break;
			
			case C_INPUT:
				BFont current;
				input->TextView()->GetFontAndColor(0, &current);
				input->TextView()->SetFontAndColor(&current, B_FONT_ALL, color);
				input->TextView()->Invalidate();
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
			/// BTextControl is stupid about font sensitivity. ///
			BString tempData (input->TextView()->Text());			
			inputFont = *font;
			input->TextView()->SetText ("Todd is a frog"); // heh.
			
			input->TextView()->SetFontAndColor (&inputFont);
			input->ResizeToPreferred();
			input->MoveTo (
				2,
				status->Frame().top - input->Frame().Height() - 1);
			
			input->TextView()->SetText (tempData.String());
		
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
	printf ("ClientWindow::NotifyRegister\n");
	BMessage msg (M_NEW_CLIENT);

	msg.AddString ("id", id.String());
	msg.AddString ("server", serverName.String());
	msg.AddInt32 ("sid", sid);
	//msg.AddPointer ("client", this);
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
	client->PostMessage (&dnsMsg);	
	
	return 0;

}

void
ClientWindow::SetupLogging (void)
{
	if (logFile.InitCheck() == B_NO_INIT && isLogging) {
	
		time_t myTime (time (0));
		struct tm *ptr;
		ptr = localtime(&myTime);
		
		app_info ai;
		be_app->GetAppInfo (&ai);
		
		BEntry entry (&ai.ref);
		BPath path;
		entry.GetPath (&path);
		path.GetParent (&path);

		BDirectory dir (path.Path());
		dir.CreateDirectory ("logs", &dir);
		path.Append ("logs");
		dir.SetTo (path.Path());
		BString sName (serverName);
		sName.ToLower();
		dir.CreateDirectory (sName.String(), &dir);
		path.Append (sName.String());
		
		BString wName (id);	// do some cleaning up
		wName.ReplaceAll ("/", "_");
		wName.ReplaceAll ("*", "_");
		wName.ReplaceAll ("?", "_");
		
		if (bowser_app->GetDateLogsState())
		{
			char tempDate[16];
			strftime (tempDate, 16, "_%Y%m%d", ptr);
			wName << tempDate;
		}
		
		wName << ".log";
		wName.ToLower();
		
		path.Append (wName.String());
		
		logFile.SetTo (path.Path(), B_READ_WRITE | B_CREATE_FILE | B_OPEN_AT_END);

		if (logFile.InitCheck() == B_NO_ERROR)
		{
			char tempTime[96];
			if (logFile.Position() == 0) // new file
			{
				strftime (tempTime, 96, "Session Start: %a %b %d %H:%M %Y\n", ptr);
			}
			else
			{
				strftime (tempTime, 96, "\n\nSession Start: %a %b %d %H:%M %Y\n", ptr);
			}	
			off_t len = strlen (tempTime);
			logFile.Write (tempTime, len);
			update_mime_info (path.Path(), false, false, true);
		}
		else
		{
			Display ("[@] Unable to make log file\n", &errorColor);
			isLogging = false;
			logFile.Unset();
			return;
		}
	}
	else if (isLogging == false && logFile.InitCheck() != B_NO_INIT) {
		time_t myTime (time (0));
		struct tm *ptr;
		ptr = localtime (&myTime);
		char tempTime[96];
		strftime (tempTime, 96, "Session Close: %a %b %d %H:%M %Y\n", ptr);
		off_t len = strlen (tempTime);
		logFile.Write (tempTime, len);
		logFile.Unset();
	}
	return;
}

