
#include "IRCDefines.h"
#include "Bowser.h"
#include "ChannelWindow.h"
#include "MessageWindow.h"
#include "StringManip.h"
#include "IgnoreWindow.h"
#include "StatusView.h"
#include "ServerWindow.h"

#include <stdio.h>

bool
ServerWindow::ParseEvents (const char *data)
{
	BString firstWord = GetWord(data, 1).ToUpper();
	BString secondWord = GetWord(data, 2).ToUpper();

	if(firstWord == "PING")
	{
		BString tempString;
		BString theServer = GetWord(data, 2);
		tempString << "PONG " << theServer;
		SendData(tempString.String());
		return true;
	}

	if (secondWord == "JOIN")
	{
		BString nick (GetNick (data));
		BString channel (GetWord (data, 3));

		channel.RemoveFirst (":");
		ClientWindow *client (Client (channel.String()));

		if (nick == myNick)
		{
			if (!client)
			{
				client =  new ChannelWindow (
					channel.String(),
					sid,
					serverName.String(),
					sMsgr,
					myNick.String());

				clients.AddItem (client);
				client->Show();
			}
				
			BString tempString ("MODE ");
			tempString << channel;
			SendData (tempString.String());
		}
		else if (client)
		{
			// someone else
			BString ident (GetIdent (data));
			BString address (GetAddress (data));
			const char *expansions[3];
			BString tempString, addy;

			expansions[0] = nick.String();
			expansions[1] = ident.String();
			expansions[2] = address.String();

			tempString = ExpandKeyed (events[E_JOIN], "NIA", expansions);
			BMessage display (M_DISPLAY);
			PackDisplay (
				&display,
				tempString.String(),
				&joinColor,
				0,
				true);

			addy << ident << "@" << address;

			BMessage aMsg (M_IS_IGNORED), reply;
			bool ignored;

			aMsg.AddString ("server", serverName.String());
			aMsg.AddString ("nick", nick.String());
			aMsg.AddString ("address", addy.String());

			be_app_messenger.SendMessage (&aMsg, &reply);
			reply.FindBool ("ignored", &ignored);
			
			BMessage msg (M_USER_ADD);
			msg.AddString ("nick",  nick.String());
			msg.AddBool ("ignore", ignored);
			msg.AddMessage ("display", &display);
			client->PostMessage (&msg);
		}
		return true;
	}
	
	if (secondWord == "PART")
	{
		BString nick (GetNick (data));
		BString channel (GetWord (data, 3));
		ClientWindow *client;

		if ((client = Client (channel.String())) != 0)
		{
			BString ident (GetIdent (data));
			BString address (GetAddress (data));
			const char *expansions[3];
			BString buffer;

			expansions[0] = nick.String();
			expansions[1] = ident.String();
			expansions[2] = address.String();

			buffer = ExpandKeyed (events[E_PART], "NIA", expansions);

			BMessage display (M_DISPLAY);
			PackDisplay (&display, buffer.String(), &joinColor, 0, true);

			BMessage msg (M_USER_QUIT);
			msg.AddString ("nick", nick.String());
			msg.AddMessage ("display", &display);
			client->PostMessage (&msg);
		}

		return true;
	}
	
	if(secondWord == "PRIVMSG")
	{
		BString theNick (GetNick (data)),
			ident (GetIdent (data)),
			address (GetAddress (data)),
			addy;


		addy << ident << "@" << address;
		BMessage aMsg (M_IS_IGNORED), reply;
		bool ignored;

		aMsg.AddString ("server", serverName.String());
		aMsg.AddString ("nick", theNick.String());
		aMsg.AddString ("address", addy.String());

		be_app_messenger.SendMessage (&aMsg, &reply);
		reply.FindBool ("ignored", &ignored);

		if (ignored)
		{
			BMessage msg (M_IGNORED_PRIVMSG);
			const char *rule;

			reply.FindString ("rule", &rule);
			msg.AddString ("nick", theNick.String());
			msg.AddString ("address", addy.String());
			msg.AddString ("rule", rule);
			Broadcast (&msg);
			return true;
		}

		BString theTarget (GetWord (data, 3).ToUpper());
		BString theMsg (RestOfString (data, 4));
		ClientWindow *client (0);

		theMsg.RemoveFirst(":");

		if(theMsg[0] == '\1' && GetWord(theMsg.String(), 1) != "\1ACTION") // CTCP
		{
			ParseCTCP (theNick, theMsg);
			return true;
		}

		if (theTarget[0] == '#')
			client = Client (theTarget.String());
		else if (!(client = Client (theNick.String())))
		{
			BString ident = GetIdent(data);
			BString address = GetAddress(data);
			BString addyString;
			addyString << ident << "@" << address;

			client = new MessageWindow (
				theNick.String(),
				sid,
				serverName.String(),
				sMsgr,
				myNick.String(),
				addyString.String());

			clients.AddItem (client);
			client->Show();
		}

		if (client) client->ChannelMessage (
			theMsg.String(),
			theNick.String(),
			ident.String(),
			address.String());

		return true;
	}
	
	if (secondWord == "NICK")
	{
		BString oldNick (GetNick (data));
		BString ident (GetIdent (data));
		BString address (GetAddress (data));
		BString newNick (GetWord (data, 3));
		const char *expansions[4];
		BString buffer;

		newNick.RemoveFirst (":");

		expansions[0] = oldNick.String();
		expansions[1] = newNick.String();
		expansions[2] = ident.String();
		expansions[3] = address.String();

		buffer = ExpandKeyed (events[E_NICK], "NnIA", expansions);
		BMessage display (M_DISPLAY);
		PackDisplay (&display, buffer.String(), &nickColor, 0, bowser_app->GetStampState());

		BMessage msg (M_CHANGE_NICK);
		msg.AddString ("oldnick", oldNick.String());
		msg.AddString ("newnick", newNick.String());
		msg.AddString ("ident", ident.String());
		msg.AddString ("address", address.String());
		msg.AddMessage ("display", &display);

		Broadcast (&msg);

		// Gotta change the server as well!
		if (myNick.ICompare (oldNick) == 0)
		{
			myNick = newNick;
			status->SetItemValue (1, newNick.String());
		}

		bowser_app->PostMessage (&msg); // for ignores

		return true;
	}

	if (secondWord == "QUIT")
	{
		BString theNick (GetNick (data).String());
		BString theRest (RestOfString (data, 3));
		BString ident (GetIdent (data));
		BString address (GetAddress (data));
		const char *expansions[4];
		BString theMsg;

		theRest.RemoveFirst (":");

		expansions[0] = theNick.String();
		expansions[1] = theRest.String();
		expansions[2] = ident.String();
		expansions[3] = address.String();

		theMsg = ExpandKeyed (events[E_QUIT], "NRIA", expansions);
		BMessage display (M_DISPLAY);
		PackDisplay (&display, theMsg.String(), &quitColor, 0, true);

		BMessage msg (M_USER_QUIT);
		msg.AddMessage ("display", &display);
		msg.AddString ("nick", theNick.String());

		Broadcast (&msg);
		return true;
	}

	if (secondWord == "KICK")
	{
		BString kicker (GetNick (data));
		BString kickee (GetWord (data, 4));
		BString rest (RestOfString (data, 5));
		BString channel (GetWord (data, 3));
		//ClientWindow *client;
		ClientWindow *client (Client (channel.String()));

		rest.RemoveFirst (":");

		if ((client = Client (channel.String())) != 0
		&&   kickee == myNick)
		{
			BMessage display (M_DISPLAY); // "you were kicked"
			BString buffer;

			buffer << "*** You have been kicked from "
				<< channel << " by " << kicker << " (" << rest << ")\n";
			PackDisplay (&display, buffer.String(), &quitColor, 0, true);

			BMessage display2 (M_DISPLAY); // "attempting auto rejoin"
			buffer = "*** Attempting to automagically rejoin ";
			buffer << channel << "...\n";
			PackDisplay (&display2, buffer.String(), &quitColor, 0, true);
			
			BMessage display3 (M_DISPLAY); // "type /join" (autorejoin off)
			buffer = "*** Type /join ";
			buffer << channel << " to rejoin channel.\n";
			PackDisplay (&display3, buffer.String(), &quitColor, 0, true);
						
			// dont need to send to server window anymore...
			// ClientWindow::MessageReceived (&display);
		
			BMessage msg (M_CHANNEL_GOT_KICKED);
			msg.AddString ("channel", channel.String());
			msg.AddMessage ("display", &display);  // "you were kicked"
			msg.AddMessage ("display2", &display2); // "attempting auto rejoin"
			msg.AddMessage ("display3", &display3); // "type /join" (autorejoin off)
			client->PostMessage (&msg);
		}

		if (client && kickee != myNick)
		{
			BMessage display (M_DISPLAY);
			const char *expansions[4];
			BString buffer;

			expansions[0] = kickee.String();
			expansions[1] = channel.String();
			expansions[2] = kicker.String();
			expansions[3] = rest.String();

			buffer = ExpandKeyed (events[E_KICK], "NCnR", expansions);
			PackDisplay (&display, buffer.String(), &quitColor, 0, true);

			BMessage msg (M_USER_QUIT);
			msg.AddString ("nick", kickee.String());
			msg.AddMessage ("display", &display);
			client->PostMessage (&msg);
		}

		return true;
	}

	if(secondWord == "TOPIC")
	{
		BString theNick (GetNick (data));
		BString theChannel (GetWord (data, 3));
		BString theTopic (RestOfString (data, 4));
		ClientWindow *client (Client (theChannel.String()));

		theTopic.RemoveFirst (":");

		if (client && client->Lock())
		{
			BString ident (GetIdent (data));
			BString address (GetAddress (data));
			const char *expansions[5];
			BString buffer;

			expansions[0] = theNick.String();
			expansions[1] = theTopic.String();
			expansions[2] = client->Id().String();
			expansions[3] = ident.String();
			expansions[4] = address.String();

			if(bowser_app->GetShowTopicState())
			{
				buffer << client->Id() << " : " << theTopic;
				client->SetTitle (buffer.String());
				client->Unlock();
			}
			else
			{
				buffer << client->Id();
				client->SetTitle (buffer.String());
				client->Unlock();
			}

			BMessage display (M_DISPLAY);

			buffer = ExpandKeyed (events[E_TOPIC], "NTCIA", expansions);
			PackDisplay (&display, buffer.String(), &whoisColor, 0, true);
			client->PostMessage (&display);
		}
		return true;
	}

	if (secondWord == "MODE")
	{
		BString theNick (GetNick (data));
		BString theChannel (GetWord (data, 3));
		BString theMode (GetWord (data, 4));
		BString theTarget (RestOfString (data, 5));
		ClientWindow *client (Client (theChannel.String()));

		if (client)
		{
			BMessage msg (M_CHANNEL_MODE);

			msg.AddString("nick", theNick.String());
			msg.AddString("mode", theMode.String());
			msg.AddString("target", theTarget.String());

			client->PostMessage (&msg);
		}
		else
		{
			BMessage msg (M_DISPLAY);
			BString buffer;
			
			theMode.RemoveFirst(":");
	
			buffer << "*** User mode changed: " << theMode << "\n";
			PackDisplay (&msg, buffer.String(), 0, 0, true);

			PostActive (&msg);
		}

		return true;
	}

	if(firstWord == "NOTICE") // _server_ notice
	{
		BString theNotice (RestOfString(data, 3));
		const char *expansions[1];

		theNotice.RemoveFirst(":");

		expansions[0] = theNotice.String();
		theNotice = ExpandKeyed (events[E_SNOTICE], "R", expansions);
		Display (theNotice.String(), 0, 0, true);

		return true;
	}

	if (firstWord == "ERROR") // server error (on connect?)
	{
		BString theError (RestOfString (data, 2));

		theError.RemoveAll (":");
		theError.Append ("\n");

		Display (theError.String(), &quitColor);
		isConnecting = false;
		return true;
	}

	if(secondWord == "NOTICE") // _user_ notice
	{
		BString theNick (GetNick (data)),
			ident (GetIdent (data)),
			address (GetAddress (data)),
			addy;

		addy << theNick << "@" << address;

		BMessage aMsg (M_IS_IGNORED), reply;
		bool ignored;

		aMsg.AddString ("server", serverName.String());
		aMsg.AddString ("nick", theNick.String());
		aMsg.AddString ("address", addy.String());

		be_app_messenger.SendMessage (&aMsg, &reply);
		reply.FindBool ("ignored", &ignored);

		if (ignored) return true;

		BString theNotice = RestOfString(data, 4);

		theNotice.RemoveFirst(":");

		if(theNotice[0] == '\1')
		{
			ParseCTCPResponse(theNick, theNotice);
			return true;
		}

		const char *expansions[4];
		BString tempString;

		expansions[0] = theNick.String();
		expansions[1] = theNotice.String();
		expansions[2] = ident.String();
		expansions[3] = address.String();

		tempString = ExpandKeyed (events[E_UNOTICE], "NRIA", expansions);
		BMessage display (M_DISPLAY);
		PackDisplay (&display, tempString.String(), &noticeColor, 0, true);
		PostActive (&display);
		return true;
	}

	return ParseENums (data, secondWord.String());
}

