#include <Menu.h>

#include "IRCDefines.h"
#include "ListWindow.h"
#include "Bowser.h"
#include "IgnoreWindow.h"
#include "NotifyWindow.h"
#include "StringManip.h"
#include "StatusView.h"
#include "ServerWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

bool
ServerWindow::ParseENums (const char *data, const char *sWord)
{
	BString secondWord (sWord);

	if (secondWord == "001") // welcome to irc messages
	{
		Lock();
		isConnected  = true;
		isConnecting = false;
		Unlock();

		BMessage msg (M_SERVER_CONNECTED);
		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);

		BString theNick = GetWord(data, 3);
		myNick = theNick;
		status->SetItemValue (1, myNick.String());

		BString theMsg = RestOfString(data, 4);
		theMsg.RemoveFirst(":");
		theMsg.Prepend("* ");
		theMsg.Append("\n");
		Display (theMsg.String(),0);

		BString title ("Bowser: ");
		title += id;
		SetTitle (title.String());
		
		return true;
	}
	
	// server info
	if (secondWord == "002"  // more welcome
	||  secondWord == "003"  // more welcome
	||  secondWord == "004"  // server strings/supported modes
	||  secondWord == "005"  // string used by UltimateIRCd
	||  secondWord == "251"  // current network users
	||  secondWord == "255"  // current/max local users
	||  secondWord == "265"  // current/max global users
	||  secondWord == "250"  // "highest connection count" info
	||  secondWord == "253"  // unknown connects (UltimateIRCd)
	||  secondWord == "266")
	{
		ParseENums(data, "001");
		return true;
	}
	

	if (secondWord == "200"  // traced link
	||  secondWord == "201"  // traced connecting
	||  secondWord == "202"  // traced handshaking
	||  secondWord == "203"  // traced unknown
	||  secondWord == "204"  // traced fellow ircop
	||  secondWord == "205"  // traced user
	||  secondWord == "206"  // traced linked server
	||  secondWord == "208"  // traced new type
	||  secondWord == "211"  // stats L info
	||  secondWord == "212"  // stats M info
	||  secondWord == "213"  // stats C info 
	||  secondWord == "214"  // stats N info
	||  secondWord == "215"  // stats I info
	||  secondWord == "216"  // stats K info
	||  secondWord == "218"  // stats Y info 
	||  secondWord == "219"  // end of stats
	||  secondWord == "241"  // stats L info 
	||  secondWord == "242"  // stats U info 
	||  secondWord == "243"  // stats O info
	||  secondWord == "244"  // stats H info
	||  secondWord == "249"  // stats P info
	||  secondWord == "256"  // admin info 
	||  secondWord == "257"  // admin info 
	||  secondWord == "258"  // admin info 
	||  secondWord == "259"  // admin info 
	||  secondWord == "261"  // trace log file
	||  secondWord == "262"  // end of trace
	||  secondWord == "302"  // userhost and usrip reply
	||  secondWord == "328")
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if (secondWord == "221") // user mode
	{
		BString theMode = GetWord(data, 4);
		BString tempString("[x] Your current mode is: ");
		tempString << theMode << '\n';

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &whoisColor);
		PostActive (&msg);

		return true;
	}

	if (secondWord == "252") // ircops
	{
		BString theNumber = GetWord(data, 4);
		BString tempString("* ");
		tempString << theNumber << " IRC Operators online\n";
		Display (tempString.String(), 0);
		return true;
	}
	
	if(secondWord == "254") // channels formed
	{
		BString theNumber = GetWord(data, 4);
		BString tempString("* ");
		tempString << theNumber << " channels formed\n";
		Display (tempString.String(), 0);
		return true;
	}


	if(secondWord == "263") // server load too busy, wait a min
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}
		
	if(secondWord == "301") // whois | away reason (5th param)
	{
		BString tempString("[x] ");
		BString theReason = RestOfString(data, 5);
		theReason.RemoveFirst(":");
		tempString << "Away: " << theReason << '\n';

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &whoisColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if (secondWord == "303") // IsOn response
	{
		BString nick (GetWord (data, 4));

		nick.RemoveFirst (":");

		BMessage msg (M_NOTIFY_END);

		msg.AddString ("nick", nick.String());
		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);
		return true;
	}

	if(secondWord == "305") // back from away
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "306") // you are now away
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}
	
	if(secondWord == "307") // whois | user has identified
	{
		BString theInfo (RestOfString (data, 5));

		theInfo.RemoveFirst (":");

		BMessage display (M_DISPLAY);
		BString buffer;

		buffer << "[x] " << theInfo << "\n";
		PackDisplay (&display, buffer.String(), &whoisColor, &serverFont);
		PostActive (&display);

		return true;
	}
	
	if (secondWord == "311") // start of whois
	{
		BString theNick (GetWord (data, 4));
		BString theIdent (GetWord (data, 5));
		BString theAddress (GetWord (data, 6));
		BString theName (RestOfString (data, 8));

		theName.RemoveFirst(":");

		BMessage display (M_DISPLAY);
		BString buffer;

		buffer << "[x] " << theNick << " (" << theIdent
			<< "@" << theAddress << ")\n" << "[x] "
			<< theName << "\n";

		PackDisplay (
			&display,
			buffer.String(),
			&whoisColor,
			&serverFont);
		PostActive (&display);

		return true;
	}

	if(secondWord == "312") // whois | server
	{
		BString theNick (GetWord (data, 4));
		BString theServer (GetWord (data, 5));
		BString theInfo (RestOfString (data, 6));

		theInfo.RemoveFirst (":");

		BMessage display (M_DISPLAY);
		BString buffer;

		buffer << "[x] Server: " << theServer << " (" << theInfo << ")\n";
		PackDisplay (&display, buffer.String(), &whoisColor, &serverFont);
		PostActive (&display);

		return true;
	}

	if (secondWord == "313") // IRC operator (uh oh)
	{
		BString theNick (GetWord (data, 4));
		
		BString tempString("[x] ");
		BString info = RestOfString(data, 5);
		info.Remove(0, 1);
		tempString << info << '\n';
		BMessage msg (M_DISPLAY);

		PackDisplay (
			&msg,
			tempString.String(),
			&whoisColor,
			&serverFont);
		PostActive (&msg);
		return true;
	}

	if (secondWord == "314") // whowas nickname info
	{
		BString theNick (GetWord (data, 4));
		BString theIdent (GetWord (data, 5));
		BString theAddress (GetWord (data, 6));
		BString theName (RestOfString(data, 8));

		BString tempString("[x] ");
		theName.RemoveFirst(":");
		tempString << theNick << " [was] (" << theIdent << "@" << theAddress << ")\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &whoisColor, &serverFont);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "315") // end of who
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "317") // whois | idle/signon (5th, 6th)
	{
		BString theNick (GetWord (data, 4));

		BString tempString("[x] ");
		BString tempString2("[x] ");
		BString theTime = GetWord(data, 5);
		BString signOnTime(GetWord(data, 6));
		tempString << "Idle time: " << theTime << " seconds\n";
		
		tempString2 << "Signon: ";
		
		
		int32 myTime = (int32)time(NULL);
		int32 serverTime = strtoul(signOnTime.String(), NULL, 0);
		if ((myTime - serverTime) < 0)
			tempString2 << theNick << " is a time traveler, or your clock is wrong\n";
		else
		{
			myTime = abs(myTime - serverTime);
		
			int32 curr = myTime / (3600 * 24); 
			if (curr)
			{
				tempString2 << curr << " day";
				if (curr > 1) tempString2 << "s";
				tempString2 << ", ";
			}
			myTime %= (3600 * 24);
			curr = myTime / 3600;
			if (curr)
			{
				tempString2 << curr << " hour";
				if (curr > 1) tempString2 << "s";
				tempString2 << ", ";
			}
			myTime %= 3600;
			curr = myTime / 60;
			if (curr)
			{
				tempString2 << curr << " minute";
				if (curr > 1) tempString2 << "s";
				tempString2 << ", ";
			}
			tempString2 << myTime % 60 << " second";
			if ((myTime % 60) != 1) tempString2 << "s";
			tempString2 << "\n";
		}
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &whoisColor, &serverFont);
		PackDisplay (&msg, tempString2.String(), &whoisColor, &serverFont);
		PostActive (&msg);

		return true;
	}

	if (secondWord == "318") // end of whois
	{
		return true;
	}

	if(secondWord == "319") // whois | channels
	{
		BString theChannels (RestOfString (data, 5));

		theChannels.RemoveFirst(":");

		BMessage display (M_DISPLAY);
		BString buffer;

		buffer << "[x] Channels: " << theChannels << "\n";
		PackDisplay (
			&display,
			buffer.String(),
			&whoisColor,
			&serverFont);
		PostActive (&display);
		return true;
	}

	if (secondWord == "321")
	{
		BMessage msg (M_LIST_BEGIN);

		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);
		return true;
	}

	if (secondWord == "322") // List command return
	{
		BMessage msg (M_LIST_EVENT);
		BString channel (GetWord (data, 4));
		BString users (GetWord (data, 5));
		BString topic (RestOfString (data, 6));

		topic.RemoveFirst (":");
		
		msg.AddString ("server", serverName.String());
		msg.AddString ("channel", channel.String());
		msg.AddString ("users", users.String());
		msg.AddString ("topic", topic.String());

		bowser_app->PostMessage (&msg);

		return true;
	}

	if (secondWord == "323") // List done
	{
		BMessage msg (M_LIST_DONE);

		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);

		return true;
	}

	if(secondWord == "324") // channel mode
	{
		BString theChan = GetWord(data, 4);
		BString theMode = GetWord(data, 5);
		BString tempStuff = RestOfString(data, 6);

		if(tempStuff != "-9z99")
		{
			theMode.Append(" ");
			theMode.Append(tempStuff); // avoid extra space w/o params
		}

		ClientWindow *aClient (ActiveClient()), 
			*theClient (Client (theChan.String())); 

		BString tempString("*** Channel mode for ");
		tempString << theChan << ": " << theMode << '\n';
		
		BMessage msg (M_CHANNEL_MODES);

		msg.AddString ("msgz", tempString.String());
		msg.AddString ("chan", theChan.String());
		msg.AddString ("mode", theMode.String());

		if (theClient)
			theClient->PostMessage (&msg);
		else if (aClient)
			aClient->PostMessage (&msg);
		else
			Display (tempString.String(), &opColor);
			
		return true;
	}

	if(secondWord == "329") // channel created
	{
		BString theChan = GetWord(data, 4);
		BString theTime = GetWord(data, 5); // long time format
		return true; // ignore, for now
	}

	if(secondWord == "331") // no topic set
	{
		BString theChan(GetWord(data, 4));
		BString tempString("[x] No topic set in ");
		tempString << theChan << '\n';

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;		
	}

	if (secondWord == "332") // channel topic
	{
		BString theChannel (GetWord (data, 4));
		BString theTopic (RestOfString (data, 5));
		ClientWindow *client (Client (theChannel.String()));

		theTopic.RemoveFirst (":");

		if (client)
		{
			BMessage display (M_DISPLAY);
			BString buffer;

			buffer << "*** Topic: " << theTopic << "\n";
			PackDisplay (&display, buffer.String(), &whoisColor, 0, bowser_app->GetStampState());

			BMessage msg (M_CHANNEL_TOPIC);

			msg.AddString ("topic", theTopic.String());
			msg.AddMessage ("display", &display);
			client->PostMessage (&msg);
		}
		return true;	
	}
	
	if (secondWord == "333") // person who changed topic
	{
		BString channel (GetWord (data, 4));
		BString user (GetWord (data, 5));
		ClientWindow *client (Client (channel.String()));

		if (client)
		{
			BMessage display (M_DISPLAY);
			BString buffer;

			buffer << "*** (Topic set by " << user << ")\n";
			PackDisplay (&display, buffer.String(), &whoisColor, 0, bowser_app->GetStampState());
			client->PostMessage (&display);
		}

		return true;
	}

	if(secondWord == "351") // version info 
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "352") // who info 
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if (secondWord == "353") // names list
	{
		BString channel (GetWord (data, 5));
		BString  names (RestOfString (data, 6));
		ClientWindow *client (Client (channel.String()));

		names.RemoveFirst (":");

		if (client) // in the channel
		{
			BMessage msg (M_CHANNEL_NAMES);
			BString nick;
			int32 place (1);

			while ((nick = GetWord (names.String(), place)) != "-9z99")
			{
				const char *sNick (nick.String());
				bool op (false),
					voice (false),
					ignored;

				if (nick[0] == '@')
				{
					++sNick;
					op = true;
				}
				else if (nick[0] == '+')
				{
					++sNick;
					voice = true;
				}

				BMessage aMsg (M_IS_IGNORED), reply;
				aMsg.AddString ("server", serverName.String());
				aMsg.AddString ("nick", sNick);

				be_app_messenger.SendMessage (&aMsg, &reply);
				reply.FindBool ("ignored", &ignored);
					
				msg.AddString ("nick", nick.String());
				msg.AddBool ("op", op);
				msg.AddBool ("voice", voice);
				msg.AddBool ("ignored", ignored);
				++place;
			}

			client->PostMessage (&msg);

			return true;
		}
		else // not in the channel
		{
			BString tempString("*** Users in ");
			tempString << channel << ": " << names << '\n';
			Display (tempString.String(), &textColor);
			return true;
		}
	}

	if (secondWord == "366"		// end of names list (don't do anything)
	||  secondWord == "369") 	// end of /whowas
	{
		return true;
	}

	if(secondWord == "371") // /info info 
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "374") // end of /info info 
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "372"  // MOTD body (standard)
	|| secondWord == "378") // MOTD body (alt)
	{
		if (!initialMotd
		||  (initialMotd && motd))
		{
			BString tempString = RestOfString(data, 4);
			tempString.RemoveFirst(":");
			tempString.Append("\n");
			Display (tempString.String(), 0);
		}
		return true;
	}

	if(secondWord == "375") // MOTD start
	{
		if (!initialMotd
		||  (initialMotd && motd))
		{
			BString tempString("- Server Message Of The Day:\n");
			Display (tempString.String(), 0);
		}
		return true;
	}
	
	if(secondWord == "376") // end of MOTD
	{
		if (!initialMotd
		||  (initialMotd && motd))
		{
			BString tempString("- End of /MOTD command.\n");

			Display (tempString.String(), 0);
			mServer->SetEnabled(true);
		}

		if (initialMotd && cmds.Length())
		{
			BMessage msg (M_SUBMIT_RAW);
			const char *place (cmds.String()), *eol;

			msg.AddBool ("lines", true);

			while ((eol = strchr (place, '\n')) != 0)
			{
				BString line;
				
				line.Append (place, eol - place);
				msg.AddString ("data", line.String());

				place = eol + 1;
			}

			if (*place)
				msg.AddString ("data", place);

			PostMessage (&msg);
		}

		initialMotd = false;
		return true;
	}

	if(secondWord == "381") // you are now irc op  
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "382") // (ircop) rehashing config  
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if (secondWord == "392") // User list started
	{
		BMessage msg (M_NOTIFY_START);

		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);
		
		return true;
	}

	if (secondWord == "393") // User list -- user
	{
		BMessage msg (M_NOTIFY_USER);
		BString buffer (RestOfString (data, 4));

		msg.AddString ("server", serverName.String());
		msg.AddString ("user", buffer.String());
		bowser_app->PostMessage (&msg);

		return true;
	}

	#if 0 // old and crap way
	if (secondWord == "394") // End of user list
	{
		
		if (notifyWindow)
			SendData ("USERS\n");

		return true;
	}
	#endif
	
	if (secondWord == "401"	// no whois avail
	||  secondWord == "402")	// no /wii avail
	{
		BString theNick (GetWord (data, 4));

		BMessage display (M_DISPLAY);
		BString buffer;

		buffer << "[x] " << theNick << " is not on IRC.\n";
		PackDisplay (
			&display,
			buffer.String(),
			&whoisColor,
			&serverFont);
		PostActive (&display);
		return true;
	}

	if(secondWord == "403") // no such channel
	{
		BString theChannel = GetWord(data, 4);
		BString tempString("[x] ");
		tempString << theChannel << " does not exist.\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "404") // cannot send to channel
	{
		BString theChannel = GetWord(data, 4);
		BString tempString("[x] Cannot send data to ");
		tempString << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "405") // too many channels
	{
		BString theChannel = GetWord(data, 4);
		BString tempString("[x] Cannot join ");
		tempString << theChannel << " (you're in too many channels.)\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "406") // whowas (not on irc)
	{
		BString theNick = GetWord(data, 4);
		BString tempString("[x] ");
		tempString << theNick << " has not been on IRC recently.\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &whoisColor, &serverFont);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "407") // duplicate recipients
	{
		BString theTarget = GetWord(data, 4);
		BString tempString("[x] There is more than one ");
		tempString << theTarget << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "412") // no text to send 
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}
	
	if(secondWord == "421") // unknown command
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "422") // motd file is missing
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
	
		if (initialMotd && cmds.Length())
		{
			BMessage msg (M_SUBMIT_RAW);
			const char *place (cmds.String()), *eol;

			msg.AddBool ("lines", true);

			while ((eol = strchr (place, '\n')) != 0)
			{
				BString line;
				
				line.Append (place, eol - place);
				msg.AddString ("data", line.String());

				place = eol + 1;
			}

			if (*place)
				msg.AddString ("data", place);

			PostMessage (&msg);
		}

		initialMotd = false;	
	
		return true;
	}

	if(secondWord == "433") // nick in use
	{
		if (isConnecting)
		{
			if (++nickAttempt >= lnicks->CountItems())
			{
				Display ("* All your pre-selected nicknames are in use.\n", 0);
				Display ("* Please type /NICK <NEWNICK> to try another.\n", 0);
				return true;
			}

			myNick = (const char *)lnicks->ItemAt (nickAttempt);
			Display ("* Nickname \"", 0);
			Display ((const char *)lnicks->ItemAt (nickAttempt - 1), 0);
			Display ("\" in use.. trying \"", 0);
			Display (myNick.String(), 0);
			Display ("\"\n", 0);

			BString tempString ("NICK ");

			tempString << myNick;
			SendData (tempString.String());

			return true;
		}

		BString theNick (GetWord (data, 4));

		BString tempString ("Nickname ");
		tempString << "[x] " << theNick << " is already in use.\n";

		BMessage display (M_DISPLAY);
		PackDisplay (&display, tempString.String(), &nickColor);
		PostActive (&display);
		return true;
	}

	if(secondWord == "441") // user not on channel
	{
		BString theChannel = GetWord(data, 5);
		BString theNick = GetWord(data, 4);
		BString tempString("[x] ");
		tempString << theNick << " is not in " << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "442") // you're not on channel
	{
		BString theChannel = GetWord(data, 4);
		BString tempString("[x] You're not in ");
		tempString << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "443") // already on channel
	{
		BString theChannel = GetWord(data, 5);
		BString theNick = GetWord(data, 4);
		BString tempString("[x] ");
		tempString << theNick << " is already in " << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "445") // summon disabled
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "446") // users disabled
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "461") // not enough parms
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "467") // key already set
	{
		BString theChannel = GetWord(data, 4);
		BString tempString("[x] Channel key already set in ");
		tempString << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "472") // unknown channel mode
	{
		BString theMode = GetWord(data, 4);
		BString tempString("[x] Unknown channel mode: '");
		tempString << theMode << "'\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "473") // cannot join channel
	{
		BString theChan = GetWord(data, 4);
		BString theReason = RestOfString(data, 5);
		theReason.RemoveFirst(":");
		theReason.ReplaceLast("channel", theChan.String());
		BString tempString("[x] ");
		tempString << theReason << " (invite only)\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "474") // cannot join channel
	{
		BString theChan = GetWord(data, 4);
		BString theReason = RestOfString(data, 5);
		theReason.RemoveFirst(":");
		theReason.ReplaceLast("channel", theChan.String());
		BString tempString("[x] ");
		tempString << theReason << " (you're banned)\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);

		return true;
	}
	
	if(secondWord == "475") // cannot join channel
	{
		BString theChan = GetWord(data, 4);
		BString theReason = RestOfString(data, 5);
		theReason.RemoveFirst(":");
		theReason.ReplaceLast("channel", theChan.String());
		BString tempString("[x] ");
		tempString << theReason << " (bad channel key)\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "481") // not an IRCop
	{
		BMessage msg (M_DISPLAY);
		BString buffer;

		PackDisplay (
			&msg,
			"[x] Permission denied: not an IRC operator.\n",
			&errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "482") // not channel operator
	{
		BString theChannel (GetWord (data, 4));

		BString tempString ("[x] You're not an operator in ");
		tempString << theChannel << ".\n";

		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &errorColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "483") // can't kill server
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "491") // "no o-lines for your host"
	{
		BString tempString("[x] ");
		tempString << RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		
		BMessage msg (M_DISPLAY);
		PackDisplay (&msg, tempString.String(), &quitColor, &serverFont);
		PostActive (&msg);
		return true;
	}

	if(secondWord == "501") // unknown user mode
	{
		BMessage msg (M_DISPLAY);
		BString buffer;
		
		buffer << "[x] Unknown MODE flag.\n";
		PackDisplay (&msg, buffer.String(), &quitColor);
		PostActive (&msg);

		return true;
	}

	if(secondWord == "502") // can't change mode for others
	{
		BMessage msg (M_DISPLAY);

		PackDisplay (
			&msg,
			"[x] Can't change mode for other users.\n",
			&errorColor);
		PostActive (&msg);

		return true;
	}

	// NON-RFC numerics:
	
	if(secondWord == "290") // MOTD-ish body
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		tempString.Prepend("- ");
		Display (tempString.String(), 0);
		return true;
	}

	if(secondWord == "291") // "Welcome" body
	{
		BString tempString = RestOfString(data, 4);
		tempString.RemoveFirst(":");
		tempString.Append("\n");
		tempString.Prepend("- ");
		Display (tempString.String(), 0);
		return true;
	}

	return false;
}

		
