#include <Application.h>
//#include <StringView.h>

//#include "IRCView.h"
//#include "StringManip.h"
#include "IRCDefines.h"
#include "ServerWindow.h"
#include "StatusView.h"
#include "MessageWindow.h"

//#include <stdio.h>
#include <stdlib.h>
//#include <net/socket.h>
#include <net/netdb.h>

MessageWindow::MessageWindow (
	const char *id_,
	int32 sid_,
	const char *serverName_,
	const BMessenger &sMsgr_,
	const char *nick,
	const char *addyString,
	bool chat,
	bool initiate,
	const char *IP,
	const char *port)

	: ClientWindow (
		id_,
		sid_,
		serverName_,
		sMsgr_,
		nick,
		BRect(110,110,730,420)),

		chatAddy (addyString ? addyString : ""),
		chatee (id_),
		dChat (chat),
		dInitiate (initiate),
		dConnected (false),
		dIP (IP),
		dPort (port)
{
	if (dChat)
	{
		id.Append (" [DCC]");

		if (dInitiate)
		{
			DCCServerSetup();
			dataThread = spawn_thread(DCCIn, "DCC Chat(I)", B_NORMAL_PRIORITY, this);
		}
		else
			dataThread = spawn_thread(DCCOut, "DCC Chat(O)", B_NORMAL_PRIORITY, this);

		resume_thread (dataThread);
	}

	status->AddItem (new StatusItem (
		serverName.String(), 0),
		true);

	status->AddItem (new StatusItem(
		"Lag: ", 0,
		STATUS_ALIGN_LEFT),
		true);
	status->AddItem (new StatusItem (
		0,
		"@@@@@@@@@@@@@@@@@",
		STATUS_ALIGN_LEFT),
		true);
	
	BMessage reply(B_REPLY);
	status->SetItemValue (STATUS_LAG, "0.000");	
	status->SetItemValue (STATUS_NICK, myNick.String());

	BString titleString (id);
	if (addyString && *addyString)
		titleString << " (" << addyString << ")";

	SetTitle(titleString.String());
	SetSizeLimits(300,2000,150,2000);


}

MessageWindow::~MessageWindow (void)
{
}


bool MessageWindow::QuitRequested (void)
{
	if (dChat)
	{
		dConnected = false;
		if (dInitiate) closesocket (mySocket);
		closesocket (acceptSocket);

		suspend_thread (dataThread);
		kill_thread (dataThread);
	}

	return ClientWindow::QuitRequested();
}

void
MessageWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_SERVER_SHUTDOWN:
		{
			// No command to IRC connection
			// Just yank her
			PostMessage (B_QUIT_REQUESTED);
			break;
		}

		case M_CHANGE_NICK:
		{
			const char *oldNick, *newNick;

			msg->FindString ("oldnick", &oldNick);
			msg->FindString ("newnick", &newNick);

			if (chatee.ICompare (oldNick) == 0)
			{
				const char *address;
				const char *ident;

				msg->FindString ("address", &address);
				msg->FindString ("ident", &ident);

				BString oldId (id);
				chatee = id = newNick;

				if (dChat) id.Append (" [DCC]");

				// SEND NOTIFY -- We could crash the deskbar
				// (who I'm I kidding -- We crashed the deskbar)
				// without it (BEFORE DISPLAYS!!!)
				BMessage bmsg (M_ID_CHANGE);

				bmsg.AddInt32 ("sid", sid);
				bmsg.AddString ("id", oldId.String());
				bmsg.AddString ("newid", id.String());
				be_app->PostMessage (&bmsg);

				BString title;
				
				title << id << " (" << ident 
					<< "@" << address << ")";
				SetTitle (title.String());

				ClientWindow::MessageReceived (msg);
			}
			else if (myNick.ICompare (oldNick) == 0)
			{
				status->SetItemValue (STATUS_NICK, newNick);
				ClientWindow::MessageReceived (msg);
			}

			break;
		}

		case M_CHANNEL_MODES:
		{
			const char *msgz;

			msg->FindString ("msgz", &msgz);
			Display (msgz, &opColor);
			break;
		}

		case M_CHANNEL_MSG:
		{
			const char *nick;

			if (msg->HasString ("nick"))
				msg->FindString ("nick", &nick);
			else
			{
				nick = chatee.String();
				msg->AddString ("nick", nick);
			}

			// This is mostly because if we send first
			// and we have open message window, this
			// will update the title bar for the address
			if (chatee.ICompare (nick) == 0
			&&  msg->HasString ("ident")
			&&  msg->HasString ("address"))
			{
				const char *ident;
				const char *address;

				msg->FindString ("ident", &ident);
				msg->FindString ("address", &address);

				BString title;

				title << id << " (" << ident
					<< "@" << address << ")";

				if (title != Title())
					SetTitle (title.String());
			}

			// Send the rest of processing up the chain
			ClientWindow::MessageReceived (msg);
			break;
		}
		
		case M_LAG_CHANGED:
		{
			BString lag;
			msg->FindString("lag", &lag);
			status->SetItemValue(STATUS_LAG, lag.String());
		}

		default:
			ClientWindow::MessageReceived (msg);
	}
}

void
MessageWindow::Show (void)
{
	ClientWindow::Show();
	SetFlags (Flags() & ~B_AVOID_FOCUS);
}

void
MessageWindow::TabExpansion (void)
{
	int32 start, finish;

	input->TextView()->GetSelection (&start, &finish);

	if (input->TextView()->TextLength()
	&&  start == finish
	&&  start == input->TextView()->TextLength())
	{
		const char *inputText (
			input->TextView()->Text()
			+ input->TextView()->TextLength());
		const char *place (inputText);
		

		while (place > input->TextView()->Text())
		{
			if (*(place - 1) == '\x20')
				break;
			--place;
		}
		
		BString insertion;

		if (!id.ICompare (place, strlen (place)))
		{
			insertion = id;
			insertion.RemoveLast(" [DCC]");
		}
		
		if (insertion.Length())
		{
			input->TextView()->Delete (
				place - input->TextView()->Text(),
				input->TextView()->TextLength());

			input->TextView()->Insert (insertion.String());
			input->TextView()->Select (
				input->TextView()->TextLength(),
				input->TextView()->TextLength());
		}
	}
}

void
MessageWindow::Parser (const char *buffer)
{
	if(!dChat)
	{
		BMessage send (M_SERVER_SEND);

		AddSend (&send, "PRIVMSG ");
		AddSend (&send, chatee);
		AddSend (&send, " :");
		AddSend (&send, buffer);
		AddSend (&send, endl);
	}
	else if (dConnected)
	{
		BString outTemp (buffer);

		outTemp << "\n";
		if (send(acceptSocket, outTemp.String(), outTemp.Length(), 0) < 0)
		{
			dConnected = false;
			Display ("DCC chat terminated.\n", 0);
			return;
		}
	}
	else
		return;

	Display ("<", 0);
	Display (myNick.String(), &myNickColor);
	Display ("> ", 0);

	BString sBuffer (buffer);

	int32 place;
	while ((place = FirstSingleKnownAs (sBuffer, chatee)) != B_ERROR)
	{
		BString tempString;

		if (place)
		{
			sBuffer.MoveInto (tempString, 0, place);
			Display (tempString.String(), 0);
		}

		sBuffer.MoveInto (tempString, 0, chatee.Length());
		Display (tempString.String(), &nickColor);

		if (atoi (autoNickTime.String()) > 0)
			nickTimes.AddItem (new TimedNick (
				chatee.String(),
				system_time()));
	}

	if (sBuffer.Length()) Display (sBuffer.String(), 0);
	Display ("\n", 0);
}

void
MessageWindow::DroppedFile (BMessage *msg)
{
	if (msg->HasRef ("refs"))
	{
		msg->what = CHOSE_FILE;
		msg->AddString ("nick", chatee.String());
		sMsgr.SendMessage (msg);
	}
}

void MessageWindow::UpdateString()
{
	if (!dChat)
	{
		BString tempString (serverName);
		tempString << " : private messaging with " << id;
		//status->SetText(tempString.String());
	}
	else
	{
		BString tempString("Private DCC chat with ");
		tempString << id;
		//status->SetText(tempString.String());
	}
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// DCC CHAT WINDOW STUFF /////////////////////////
//////////////////////////////////////////////////////////////////////////////

void
MessageWindow::DCCServerSetup (void)
{
	int32 myPort (1500 + (rand() % 5000));
	struct sockaddr_in sa;
	
	mySocket = socket (AF_INET, SOCK_STREAM, 0);

	if (mySocket < 0)
	{
		Display ("Error creating socket.\n", 0);
		return;
	}

	BLooper *looper;
	ServerWindow *server;
	sMsgr.Target (&looper);
	server = dynamic_cast<ServerWindow *>(looper);

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = server->LocalAddress();
	sa.sin_port = htons(myPort);
	if (bind (mySocket, (struct sockaddr*)&sa, sizeof(sa)) == -1)
	{
		myPort = 1500 + (rand() % 5000); // try once more
		sa.sin_port = htons(myPort);
		if (bind (mySocket, (struct sockaddr*)&sa, sizeof(sa)) == -1)
		{
			Display ("Error binding socket.\n", 0);
			return;
		}
	}

	BMessage msg (M_SERVER_SEND);
	BString buffer;

	buffer << "PRIVMSG " << chatee << " :\1DCC CHAT chat ";
	buffer << htonl (server->LocalAddress()) << " ";
	buffer << myPort << "\1";
	msg.AddString ("data", buffer.String());
	sMsgr.SendMessage (&msg);
	
	listen (mySocket, 1);

	{
		BMessage msg (M_DISPLAY);
		BString buffer;
		struct in_addr addr;
		
		addr.s_addr = server->LocalAddress();
		buffer << "Accepting connection on address "
			<< inet_ntoa (addr) << ", port " << myPort << "\n";

		Display (buffer.String(), 0);
	}
}


int32
MessageWindow::DCCIn (void *arg)
{
	MessageWindow *mWin ((MessageWindow *)arg);
	BMessenger sMsgr (mWin->sMsgr);
	BMessenger mMsgr (mWin);

	struct sockaddr_in remoteAddy;
	int theLen (sizeof (struct sockaddr_in));

	mWin->acceptSocket = accept(mWin->mySocket, (struct sockaddr*)&remoteAddy, &theLen);

	mWin->dConnected = true;

	BMessage msg (M_DISPLAY);

	mWin->PackDisplay (&msg, "Connected!\n", 0);
	mWin->PostMessage (&msg);

	char tempBuffer[2];
	BString inputBuffer;
	
	while(mWin->dConnected)
	{
		snooze(10000);
		while (mWin->dConnected && mWin->DataWaiting())
		{
			if(recv(mWin->acceptSocket, tempBuffer, 1, 0) == 0)
			{
				BMessage msg (M_DISPLAY);

				mWin->dConnected = false;
				mWin->PackDisplay (&msg, "DCC chat terminated.\n", 0);
				mWin->PostMessage (&msg);
				goto outta_there; // I hate goto, but this is a good use.
			}

			if (tempBuffer[0] == '\n')
			{
				mWin->ChannelMessage (inputBuffer.String());
				inputBuffer = "";
			}
			else
			{
				inputBuffer.Append(tempBuffer[0],1);
			}
		}
	}
	
	outta_there: // GOTO MARKER

	closesocket (mWin->mySocket);
	closesocket (mWin->acceptSocket);

	return 0;
}

int32
MessageWindow::DCCOut (void *arg)
{
	MessageWindow *mWin ((MessageWindow *)arg);
	struct sockaddr_in sa;
	int status;
	char *endpoint;
	u_long realIP = strtoul(mWin->dIP.String(), &endpoint, 10);
	
	if ((mWin->acceptSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		BMessage msg (M_DISPLAY);

		mWin->PackDisplay (&msg, "Error opening socket.\n", 0);
		mWin->PostMessage (&msg);
		return false;
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons (atoi (mWin->dPort.String()));
	sa.sin_addr.s_addr = ntohl (realIP);
	memset(sa.sin_zero, 0, sizeof(sa.sin_zero));

	{
		BMessage msg (M_DISPLAY);
		BString buffer;
		struct in_addr addr;

		addr.s_addr = ntohl (realIP);

		buffer << "Trying to connect to address "
			<< inet_ntoa (addr)
			<< " port " << mWin->dPort << "\n";
		mWin->PackDisplay (&msg, buffer.String(), 0);
		mWin->PostMessage (&msg);
	}

	status = connect (mWin->acceptSocket, (struct sockaddr *)&sa, sizeof(sa));
	if(status < 0)
	{
		BMessage msg (M_DISPLAY);

		mWin->PackDisplay (&msg, "Error connecting socket.\n", 0);
		mWin->PostMessage (&msg);
		closesocket (mWin->mySocket);
		return false;
	}

	mWin->dConnected = true;

	BMessage msg (M_DISPLAY);
	mWin->PackDisplay (&msg, "Connected!\n", 0);
	mWin->PostMessage (&msg);

	char tempBuffer[2];
	BString inputBuffer;
	
	while (mWin->dConnected)
	{
		snooze(10000);
		while (mWin->dConnected && mWin->DataWaiting())
		{
			if(recv (mWin->acceptSocket, tempBuffer, 1, 0) == 0)
			{
				BMessage msg (M_DISPLAY);

				mWin->dConnected = false;
				mWin->PackDisplay (&msg, "DCC chat terminated.\n", 0);
				mWin->PostMessage (&msg);
				goto outta_loop; // I hate goto, but this is a good use.
			}

			if(tempBuffer[0] == '\n')
			{
				mWin->ChannelMessage (inputBuffer.String());
				inputBuffer = "";
			}
			else
			{
				inputBuffer.Append(tempBuffer[0],1);
			}
		}
	}
	
	outta_loop: // GOTO MARKER

	closesocket (mWin->acceptSocket);

	return 0;
}

bool MessageWindow::DataWaiting()
{
	struct fd_set fds;
	FD_ZERO(&fds);
	FD_SET(acceptSocket, &fds);
	select(acceptSocket + 1, &fds, NULL, NULL, 0);
	return FD_ISSET(acceptSocket, &fds);
}
