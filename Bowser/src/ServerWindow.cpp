
#include <NetEndpoint.h>
#include <NetAddress.h>
#include <UTF8.h>
#include <Path.h>
#include <Alert.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Autolock.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Bowser.h"
#include "IRCView.h"
#include "StringManip.h"
#include "ChannelWindow.h"
#include "DCCConnect.h"
#include "MessageWindow.h"
#include "ListWindow.h"
#include "IgnoreWindow.h"
#include "NotifyWindow.h"
#include "StatusView.h"
#include "ServerWindow.h"

int32 ServerWindow::ServerSeed			= 0;
BLocker ServerWindow::identLock;

ServerWindow::ServerWindow (
	const char *id_,

	BList *nicks,
	const char *port,
	const char *name,
	const char *ident,
	const char **events_,
	bool motd_,
	bool identd_,
	const char *cmds_)

	: ClientWindow (
		id_,
		ServerSeed++,
		id_,
		(const char *)nicks->FirstItem(),
		BRect(105,105,550,400)),

		lnicks (nicks),
		lport (port),
		lname (name),
		lident (ident),
		nickAttempt (0),
		myNick ((const char *)nicks->FirstItem()),
		isConnected (false),
		isConnecting (true),
		hasWarned (false),
		isQuitting (false),
		endPoint (0),
		send_buffer (0),
		send_size (0),
		parse_buffer (0),
		parse_size (0),
		events (events_),
		motd (motd_),
		initialMotd (true),
		identd (identd_),
		cmds (cmds_)
{
	SetSizeLimits(200,2000,150,2000);

	SetTitle ("Bowser: Connecting");

	myFont = *(bowser_app->GetClientFont (F_SERVER));
	ctcpReqColor = bowser_app->GetColor (C_CTCP_REQ);
	ctcpRpyColor = bowser_app->GetColor (C_CTCP_RPY);
	whoisColor   = bowser_app->GetColor (C_WHOIS);
	errorColor   = bowser_app->GetColor (C_ERROR);
	quitColor    = bowser_app->GetColor (C_QUIT);
	joinColor    = bowser_app->GetColor (C_JOIN);
	noticeColor  = bowser_app->GetColor (C_NOTICE);
	textColor    = bowser_app->GetColor (C_SERVER);
	
	AddShortcut('W', B_COMMAND_KEY, new BMessage(M_SERVER_ALTW)); 

	status->AddItem (new StatusItem (
		serverName.String(), 0),
		true);

	status->AddItem (new StatusItem (
		0,
		"@@@@@@@@@@@@@@@@@",
		STATUS_ALIGN_LEFT),
		true);
	status->SetItemValue (STATUS_NICK, myNick.String());

	// We pack it all up and ship it off to the
	// the establish thread.  Establish can
	// work in getting connecting regardless of
	// what this instance of ServerWindow is doing
	BMessage *msg (new BMessage);
	msg->AddString ("id", id.String());
	msg->AddString ("port", lport.String());
	msg->AddString ("ident", lident.String());
	msg->AddString ("name", lname.String());
	msg->AddString ("nick", myNick.String());
	msg->AddBool ("identd", identd);
	msg->AddPointer ("server", this);

	loginThread = spawn_thread (
		Establish,
		"complimentary_tote_bag",
		B_NORMAL_PRIORITY,
		msg);

	resume_thread (loginThread);

	BMessage aMsg (M_SERVER_STARTUP);
	aMsg.AddString ("server", serverName.String());
	bowser_app->PostMessage (&aMsg);
}

ServerWindow::~ServerWindow (void)
{
	if (endPoint)     delete endPoint;
	if (send_buffer)  delete [] send_buffer;
	if (parse_buffer) delete [] parse_buffer;

	char *nick;
	while ((nick = (char *)lnicks->RemoveItem (0L)) != 0)
		delete [] nick;
	delete lnicks;

	for (int32 i = 0; i < MAX_EVENTS; ++i)
		delete [] events[i];
	delete [] events;
}


bool
ServerWindow::QuitRequested()
{
	BMessage *msg (CurrentMessage());
	bool shutdown (false);

	if (msg->HasBool ("bowser:shutdown"))
		msg->FindBool ("bowser:shutdown", &shutdown);

	if (isConnecting && !hasWarned && !shutdown)
	{
		Display ("* To avoid network instability, it is HIGHLY recommended\n", 0);
		Display ("* that you have finished the connection process before\n", 0);
		Display ("* closing Bowser. If you _still_ want to try to quit,\n", 0);
		Display ("* click the close button again.\n", 0);

		hasWarned = true;
		return false;
	}

	if (msg->HasString ("bowser:quit"))
	{
		const char *quitstr;

		msg->FindString ("bowser:quit", &quitstr);
		quitMsg = quitstr;
	}

	bool sentDemise (false);

	for (int32 i = 0; i < clients.CountItems(); ++i)
	{
		ClientWindow *client ((ClientWindow *)clients.ItemAt (i));

		if (client != this)
		{
			BMessage msg (B_QUIT_REQUESTED);

			msg.AddBool ("bowser:part", false);
			client->PostMessage (&msg);

			sentDemise = true;
		}
	}

	isQuitting = true;
	if (sentDemise)
		return false;

	if (endPoint)
	{
		if (quitMsg.Length() == 0)
		{
			const char *expansions[1];
			BString version (VERSION);

			expansions[0] = version.String();
			quitMsg << "QUIT :" << ExpandKeyed (bowser_app->GetCommand (CMD_QUIT).String(), "V", expansions);
		}

		SendData (quitMsg.String());
	}

	// Don't kill login thread.. it will figure
	// things out for itself and very quickly
	kill_thread(loginThread);
	// Tell the app about our death, he may care
	BMessage aMsg (M_SERVER_SHUTDOWN);
	aMsg.AddString ("server", serverName.String());
	bowser_app->PostMessage (&aMsg);
	
	if (bowser_app->GetShowSetupState())
	{
		bowser_app->PostMessage (M_SETUP_ACTIVATE);
	}
		
	return ClientWindow::QuitRequested();
}

void
ServerWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_PARSE_LINE:
		{
			const char *buffer;

			msg->FindString ("line", &buffer);
			ParseLine (buffer);

			break;
		}

		// Client's telling us it closed
		case M_CLIENT_SHUTDOWN:
		{
			ClientWindow *client;

			msg->FindPointer ("client", reinterpret_cast<void **>(&client));
			clients.RemoveItem (client);

			if (isQuitting && clients.CountItems() <= 1)
				PostMessage (B_QUIT_REQUESTED);

			break;
		}

		case M_SERVER_SEND:
		{
			BString buffer;
			int32 i;

			for (i = 0; msg->HasString ("data", i); ++i)
			{
				const char *str;

				msg->FindString ("data", i, &str);
				buffer << str;
			}

			SendData (buffer.String());

			break;
		}

		case CYCLE_WINDOWS:
		{
			ClientWindow *client;
			bool found (false);

			msg->FindPointer ("client", reinterpret_cast<void **>(&client));

			for (int32 i = 0; i < clients.CountItems(); ++i)
				if (client == (ClientWindow *)clients.ItemAt (i)
				&&  i != clients.CountItems() - 1)
				{
					found = true;
					client = (ClientWindow *)clients.ItemAt (i + 1);
					break;
				}

			if (!found)
				client = (ClientWindow *)clients.FirstItem();
					
			client->Activate();
			break;
		}

		case CYCLE_BACK:
		{
			ClientWindow *client, *last (0);

			msg->FindPointer ("client", reinterpret_cast<void **>(&client));
			
			for (int32 i = 0; i < clients.CountItems(); ++i)
			{
				if (client == (ClientWindow *)clients.ItemAt (i))
					break;

				last = (ClientWindow *)clients.ItemAt (i);
			}

			if (!last)
				last = (ClientWindow *)clients.LastItem();
			last->Activate();
			break;
		}

		case OPEN_MWINDOW:
		{
			ClientWindow *client;
			const char *theNick;

			msg->FindString ("nick", &theNick);

			if (!(client = Client (theNick)))
			{
				client = new MessageWindow (
					theNick,
					sid,
					serverName.String(),
					sMsgr,
					myNick.String(),
					"");

				clients.AddItem (client);
				client->Show();
			}
			else
				client->Activate (true);

			if (msg->HasMessage ("msg"))
			{
				BMessage buffer;

				msg->FindMessage ("msg", &buffer);
				client->PostMessage (&buffer);
			}
			
			break;
		}

		case DCC_ACCEPT:
		{
			bool cont (false);
			const char *nick,
				*size,
				*ip,
				*port;
			BPath path;

			msg->FindString("bowser:nick", &nick);
			msg->FindString("bowser:size", &size);
			msg->FindString("bowser:ip", &ip);
			msg->FindString("bowser:port", &port);

			if (msg->HasString ("path"))
				path.SetTo (msg->FindString ("path"));
			else
			{
				const char *file;
				entry_ref ref;

				msg->FindRef ("directory", &ref);
				msg->FindString("name", &file);

				BDirectory dir (&ref);
				path.SetTo (&dir, file);
			}

			if (msg->HasBool ("continue"))
				msg->FindBool ("continue", &cont);

			DCCReceive *view;
			view = new DCCReceive (
				nick,
				path.Path(),
				size,
				ip,
				port,
				cont);

			BMessage aMsg (M_DCC_FILE_WIN);
			aMsg.AddPointer ("view", view);
			be_app->PostMessage (&aMsg);
			break;
		}


		case B_CANCEL:

			if (msg->HasPointer ("source"))
			{
				BFilePanel *fPanel;

				msg->FindPointer ("source", reinterpret_cast<void **>(&fPanel));
				delete fPanel;
			}
			break;

		case CHAT_ACCEPT:
		{
			int32 acceptDeny;
			const char *theNick, *theIP, *thePort;
			msg->FindInt32("which", &acceptDeny);
			if(acceptDeny)
				return;
			msg->FindString("nick", &theNick);
			msg->FindString("ip", &theIP);
			msg->FindString("port", &thePort);


			MessageWindow *window (new MessageWindow (
				theNick,
				sid,
				serverName.String(),
				sMsgr,
				myNick.String(),
				"",
				true,
				false,
				theIP,
				thePort));
			clients.AddItem (window);
			window->Show();
			break;	
		}

		case CHAT_ACTION: // DCC chat
		{
			ClientWindow *client;
			const char *theNick;
			BString theId;

			msg->FindString ("nick", &theNick);
			theId << theNick << " [DCC]";

			if ((client = Client (theId.String())) == 0)
			{
				client = new MessageWindow (
					theNick,
					sid,
					serverName.String(),
					sMsgr,
					myNick.String(),
					"",
					true,
					true);

				clients.AddItem (client);
				client->Show();
			}

			break;
		}

		case M_SERVER_ALTW:
		{
		
			if (bowser_app->GetAltwServerState())
			{
				sMsgr = BMessenger (this);
				BMessage msg (B_QUIT_REQUESTED);
				msg.AddString ("bowser:quit", "not_needed_in_this_case");
				sMsgr.SendMessage (&msg);
			}
			
			break;
		}

		case CHOSE_FILE: // DCC send
		{
			const char *nick;
			entry_ref ref;
			off_t size;
			msg->FindString ("nick", &nick);
			msg->FindRef ("refs", &ref); // get file
			
			BEntry entry (&ref);
			BPath path (&entry);
			printf("file path: %s\n", path.Path());
			entry.GetSize (&size);

			BString ssize;
			ssize << size;

			// because of a bug in the be library
			// we have to get the sockname on this
			// socket, and not the one that DCCSend
			// binds.  calling getsockname on a
			// a binded socket will return the
			// LAN ip over the DUN one 
			struct sockaddr_in sin;
			int sinsize (sizeof (struct sockaddr_in));

			getsockname (endPoint->Socket(), (struct sockaddr *)&sin, &sinsize);			
			
			DCCSend *view;
			view = new DCCSend (
				nick,
				path.Path(),
				ssize.String(),
				sMsgr,
				sin.sin_addr);
			BMessage msg (M_DCC_FILE_WIN);
			msg.AddPointer ("view", view);
			be_app->PostMessage (&msg);
			break;
		}


		default:
			ClientWindow::MessageReceived (msg);
	}
}

void
ServerWindow::MenusBeginning (void)
{
	mIgnore->SetEnabled (isConnected);
	mList->SetEnabled (isConnected);
	mNotifyWindow->SetEnabled (isConnected);

	ClientWindow::MenusBeginning();
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////// PROGRAM FUNCTIONS ///////////////////////////
///////////////////////////////////////////////////////////////////////////

int32
ServerWindow::Establish (void *arg)
{
	BMessage *msg (reinterpret_cast<BMessage *>(arg));
	const char *id, *port, *ident, *name, *nick;
	ServerWindow *server;
	bool identd;
	
	if(bowser_app->GetHideSetupState())
	{
		bowser_app->PostMessage (M_SETUP_HIDE);
	}

	msg->FindString ("id", &id);
	msg->FindString ("port", &port);
	msg->FindString ("ident", &ident);
	msg->FindString ("name", &name);
	msg->FindString ("nick", &nick);
	msg->FindBool ("identd", &identd);
	msg->FindPointer ("server", reinterpret_cast<void **>(&server));

	BNetAddress address;
	BMessenger msgr (server);

	if (address.SetTo (id, atoi (port)) != B_NO_ERROR)
	{
		BString buffer;

		buffer << "The address \"" << id << "\" and port \""
			<< port << "\" seem to have a problem.  I can't figure "
			"it out.";

		BAlert *alert (new BAlert (
			"Invalid Address",
			buffer.String(),
			"Sheesh",
			0,
			0,
			B_WIDTH_AS_USUAL,
			B_STOP_ALERT));
		alert->Go();

		if (msgr.LockTarget())
		{
			server->isConnecting = false;
			server->Unlock();
		}

		msgr.SendMessage (B_QUIT_REQUESTED);
		return 0;
	}
	
	BNetEndpoint *endPoint (new BNetEndpoint);

	if (!endPoint || endPoint->InitCheck() != B_NO_ERROR)
	{
		BString buffer;

		buffer << "Cannot create a connection to address "
			<< "\"" << id << "\" and port \""
			<< port << "\".  I can't figure it out.";

		BAlert *alert (new BAlert (
			"Invalid Address",
			buffer.String(),
			"Sheesh",
			0,
			0,
			B_WIDTH_AS_USUAL,
			B_STOP_ALERT));
		alert->Go();

		if (msgr.LockTarget())
		{
			server->isConnecting = false;
			server->Unlock();
		}

		if (endPoint) delete endPoint;
		return 0;
	}

	// just see if he's still hanging around before
	// we got blocked for a minute
	if (!msgr.IsValid())
	{
		delete endPoint;
		return 0;
	}

	identLock.Lock();
	if (endPoint->Connect (address) == B_NO_ERROR)
	{
		struct sockaddr_in sin;
		int namelen (sizeof (struct sockaddr_in));

		// Here we save off the local address for DCC and stuff
		// (Need to make sure that the address we use to connect
		//  is the one that we use to accept on)
		getsockname (endPoint->Socket(), (struct sockaddr *)&sin, &namelen);
		server->localAddress = sin.sin_addr.s_addr;

		if (identd)
		{
			BNetEndpoint identPoint, *accepted;
			BNetAddress identAddress (sin.sin_addr, 113);
			BNetBuffer buffer;
			char received[64];

			if (msgr.IsValid()
			&&  identPoint.InitCheck()             == B_OK
			&&  identPoint.Bind (identAddress)     == B_OK
			&&  identPoint.Listen()                == B_OK
			&& (accepted = identPoint.Accept())    != 0
			&&  accepted->Receive (buffer, 64)     >= 0
			&&  buffer.RemoveString (received, 64) == B_OK)
			{
				int32 len;
	
				received[63] = 0;
				while ((len = strlen (received))
				&&     isspace (received[len - 1]))
					received[len - 1] = 0;

				BNetBuffer output;
				BString string;

				string.Append (received);
				string.Append (" : USERID : BeOS : ");
				string.Append (ident);
				string.Append ("\r\n");

				output.AppendString (string.String());
				accepted->Send (output);
				delete accepted;
			}
		}

		BString string;

		string = "USER ";
		string.Append (ident);
		string.Append (" localhost ");
		string.Append (id);
		string.Append (" :");
		string.Append (name);


		if (msgr.LockTarget())
		{
			server->endPoint = endPoint;
			server->SendData (string.String());

			string = "NICK ";
			string.Append (nick);
			server->SendData (string.String());
			server->Unlock();
		}
		identLock.Unlock();
	}

	else // No endpoint->connect
	{
		identLock.Unlock();
		BAlert *alert (new BAlert (
			"Establish Failed",
			"Could not establish a connection to server.  "
			"Are you sure your phone is plugged into the "
			"the phone jack?  If you have an external modem, "
			"is it on?  Did you pay the phone bill?",
			"Oh well",
			0,
			0,
			B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT));

		alert->Go();

		if (msgr.LockTarget())
		{
			server->isConnecting = false;
			server->Unlock();
		}

		msgr.SendMessage (B_QUIT_REQUESTED);
		delete endPoint;

		return 0;
	}
		
	
	// Don't need this anymore
	delete msg;
	struct fd_set eset, rset, wset;
	struct timeval tv = {0, 0};

	FD_ZERO (&eset);
	FD_ZERO (&rset);
	FD_ZERO (&wset);

	BString buffer;

	while (msgr.LockTarget())
	{
		BNetBuffer input (1024);
		int32 length (0);

		FD_SET (endPoint->Socket(), &eset);
		FD_SET (endPoint->Socket(), &rset);
		FD_SET (endPoint->Socket(), &wset);

		if (select (endPoint->Socket() + 1, &rset, 0, &eset, &tv) > 0
		&&  FD_ISSET (endPoint->Socket(), &rset))
		{
			if ((length = endPoint->Receive (input, 1024)) > 0)
			{
				BString temp;
				int32 index;
	
				temp.SetTo ((char *)input.Data(), input.Size());
				buffer += temp;
	
				while ((index = buffer.FindFirst ('\n')) != B_ERROR)
				{
					temp.SetTo (buffer, index);
					buffer.Remove (0, index + 1);
	
					temp.RemoveLast ("\r");
		
					#ifdef DEV_BUILD
	
					if (DumpReceived)
					{
						printf ("RECEIVED: (%ld) \"", temp.Length());
						for (int32 i = 0; i < temp.Length(); ++i)
						{
							if (isprint (temp[i]))
								printf ("%c", temp[i]);
							else
								printf ("[0x%02x]", temp[i]);
						}
						printf ("\"\n");
					}
	
					#endif
		
					// We ship it off this way because
					// we want this thread to loop relatively
					// quickly.  Let ServerWindow's main thread
					// handle the processing of incoming data!
					BMessage msg (M_PARSE_LINE);
					msg.AddString ("line", temp.String());
					msgr.SendMessage (&msg);
				}
			}
	
			if (FD_ISSET (endPoint->Socket(), &eset)
			|| (FD_ISSET (endPoint->Socket(), &rset) && length == 0)
			|| !FD_ISSET (endPoint->Socket(), &wset)
			|| length < 0)
			{
				// TODO Alert box baby!
				printf ("Negative from endpoint receive! (%ld)\n", length);
				printf ("(%d) %s\n",
					endPoint->Error(),
					endPoint->ErrorStr());

				printf ("eset : %s\nrset: %s\nwset: %s\n",
					FD_ISSET (endPoint->Socket(), &eset) ? "true" : "false",
					FD_ISSET (endPoint->Socket(), &rset) ? "true" : "false",
					FD_ISSET (endPoint->Socket(), &wset) ? "true" : "false");
					
	
				msgr.SendMessage (B_QUIT_REQUESTED);
				server->Unlock();
				break;
			}
		}
		server->Unlock();

		// take a nap, so the ServerWindow can do things
		snooze (20000);
	}

	return 0;
}

void
ServerWindow::SendData (const char *cData)
{
	int32 length;
	BString data (cData);

	data.Append("\r\n");
	length = data.Length() + 1;

	// The most it could be is that every
	// stinking character is a utf8 character.
	// Which can be at most 3 bytes.  Hence
	// our multiplier of 3

	if (send_size < length * 3UL)
	{
		if (send_buffer) delete [] send_buffer;
		send_buffer = new char [length * 3];
		send_size = length * 3;
	}

	int32 dest_length (send_size), state (0);

	convert_from_utf8 (
		B_ISO1_CONVERSION,
		data.String(), 
		&length,
		send_buffer,
		&dest_length,
		&state);

	if (endPoint == 0
	|| (length = endPoint->Send (send_buffer, strlen (send_buffer))) < 0)
			PostMessage (B_QUIT_REQUESTED);
	
	#ifdef DEV_BUILD
	if (DumpSent) printf("SENT: (%ld) %s", length, data.String());
	#endif
}

void
ServerWindow::ParseLine (const char *cData)
{
	BString data (FilterCrap (cData));

	int32 length (data.Length() + 1);

	if (parse_size < length * 3UL)
	{
		if (parse_buffer) delete [] parse_buffer;
		parse_buffer = new char [length * 3];
		parse_size = length * 3;
	}

	int32 dest_length (parse_size), state (0);

	convert_to_utf8 (
		B_ISO1_CONVERSION,
		data.String(), 
		&length,
		parse_buffer,
		&dest_length,
		&state);

	if (ParseEvents (parse_buffer))
		return;

	data.Append("\n");
	Display (data.String(), 0);
}

ClientWindow *
ServerWindow::Client (const char *cName)
{
	ClientWindow *client (0);

	for (int32 i = 0; i < clients.CountItems(); ++i)
	{
		ClientWindow *item ((ClientWindow *)clients.ItemAt (i));

		if (strcasecmp (cName, item->Id().String()) == 0)
		{
			client = item;
			break;
		}
	}

	return client;
}

ClientWindow *
ServerWindow::ActiveClient (void)
{
	ClientWindow *client (0);

	for (int32 i = 0; i < clients.CountItems(); ++i)
		if (((ClientWindow *)clients.ItemAt (i))->IsActive())
			client = (ClientWindow *)clients.ItemAt (i);

	return client;
}

void
ServerWindow::Broadcast (BMessage *msg)
{
	for (int32 i = 0; i < clients.CountItems(); ++i)
	{
		ClientWindow *client ((ClientWindow *)clients.ItemAt (i));

		if (client != this)
			client->PostMessage (msg);
	}
}

void
ServerWindow::RepliedBroadcast (BMessage *msg)
{
	BMessage cMsg (*msg);
	BAutolock lock (this);

	for (int32 i = 0; i < clients.CountItems(); ++i)
	{
		ClientWindow *client ((ClientWindow *)clients.ItemAt (i));

		if (client != this)
		{
			BMessenger msgr (client);
			BMessage reply;

			msgr.SendMessage (&cMsg, &reply);
		}
	}
}


void
ServerWindow::DisplayAll (const char *buffer, bool channelOnly)
{
	for (int32 i = 0; i < clients.CountItems(); ++i)
	{
		ClientWindow *client ((ClientWindow *)clients.ItemAt (i));

		if (!channelOnly || dynamic_cast<ChannelWindow *>(client))
		{
			BMessage msg (M_DISPLAY);

			PackDisplay (&msg, buffer);

			client->PostMessage (&msg);
		}
	}

	return;
}

void
ServerWindow::PostActive (BMessage *msg)
{
	BAutolock lock (this);
	ClientWindow *client (ActiveClient());

	if (client)
		client->PostMessage (msg);
	else
		ServerWindow::MessageReceived (msg);
}


BString
ServerWindow::FilterCrap (const char *data)
{
	BString outData("");
	int32 theChars (strlen (data));

	for (int32 i = 0; i < theChars; ++i)
	{
		if(data[i] > 1 && data[i] < 32)
		{
			// TODO Get these codes working
			if(data[i] == 3)
			{
				#ifdef DEV_BUILD
				if (ViewCodes)
					outData << "[0x03]{";
				#endif

				++i;
				while (i < theChars
				&&   ((data[i] >= '0'
				&&     data[i] <= '9')
				||     data[i] == ','))
				{
					#ifdef DEV_BUILD
					if (ViewCodes)
						outData << data[i];
					#endif

					++i;
				}
				--i;

				#ifdef DEV_BUILD
				if (ViewCodes)
					outData << "}";
				#endif
			}

			#ifdef DEV_BUILD
			else if (ViewCodes)
			{
				char buffer[16];

				sprintf (buffer, "[0x%02x]", data[i]);
				outData << buffer;
			}
			#endif
		}

		else outData << data[i];
	}
	
	return outData;
}

void
ServerWindow::StateChange (BMessage *msg)
{
	// Important to call ClientWindow's State change first
	// We correct some things it sets
	ClientWindow::StateChange (msg);

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
			case C_CTCP_REQ:
				ctcpReqColor = *color;
				break;

			case C_CTCP_RPY:
				ctcpRpyColor = *color;
				break;

			case C_WHOIS:
				whoisColor = *color;
				break;

			case C_ERROR:
				errorColor = *color;
				break;

			case C_QUIT:
				quitColor = *color;
				break;

			case C_JOIN:
				joinColor = *color;
				break;

			case C_NOTICE:
				noticeColor = *color;
				break;
		}
	}

	if (msg->HasPointer ("font"))
	{
		BFont *font;
		int32 which;

		msg->FindInt32 ("which", &which);
		msg->FindPointer ("font", reinterpret_cast<void **>(&font));

		// ClientWindow may have screwed us, make it right
		if (which == F_TEXT)
			myFont = serverFont;
	}

	if (msg->HasString ("event"))
	{
		const char *event;
		int32 which;

		msg->FindInt32 ("which", &which);
		msg->FindString ("event", &event);

		delete [] events[which];
		events[which] = strcpy (new char [strlen (event) + 1], event);
	}
}

void
ServerWindow::AddResumeData (BMessage *msg)
{
	ResumeData *data;

	data = new ResumeData;

	data->expire = system_time() + 50000000LL;
	data->nick   = msg->FindString ("bowser:nick");
	data->file   = msg->FindString ("bowser:file");
	data->size   = msg->FindString ("bowser:size");
	data->ip     = msg->FindString ("bowser:ip");
	data->port   = msg->FindString ("bowser:port");
	data->path   = msg->FindString ("path");
	data->pos    = msg->FindInt64  ("pos");

	resumes.AddItem (data);

	BString buffer;

	buffer << "PRIVMSG "
		<< data->nick
		<< " :\1DCC RESUME "
		<< data->file
		<< " "
		<< data->port
		<< " "
		<< data->pos
		<< "\1";

	SendData (buffer.String());
}

uint32
ServerWindow::LocalAddress (void) const
{
	return localAddress;
}

