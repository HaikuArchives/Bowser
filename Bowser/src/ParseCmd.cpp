
#include <FilePanel.h>
#include <Path.h>
#include <Roster.h>
#include <FindDirectory.h>
#include <stdio.h>
#include <map.h>
#include <netdb.h>
#include <ctype.h>

#include "Bowser.h"
#include "IRCDefines.h"
#include "ChannelWindow.h"
#include "MessageWindow.h"
#include "ServerWindow.h"
#include "StringManip.h"
#include "Names.h"
#include "IgnoreWindow.h"
#include "ClientWindow.h"
#include "IRCView.h"
#include "IRCDefines.h"
#include "Settings.h"

bool
ClientWindow::ParseCmd (const char *data)
{
	BString firstWord (GetWord(data, 1).ToUpper());
		


	if (firstWord == "/KILL"	// we need to insert a ':' before parm2
	||  firstWord == "/SQUIT"   // for the user
	||  firstWord == "/PRIVMSG"
	||  firstWord == "/WALLOPS")
	{
		BString theCmd (firstWord.RemoveAll ("/")),
	            theRest (RestOfString (data, 2));
		
		BMessage send (M_SERVER_SEND);
		AddSend (&send, theCmd);
		if (theRest != "-9z99")
		{
			AddSend (&send, " :");
			AddSend (&send, theRest);
		}
		AddSend (&send, endl);	

		return true;
	}


	// some quick aliases for scripts, these will of course be
	// moved to an aliases section eventually
	
	if (firstWord == "/SOUNDPLAY"
	||  firstWord == "/CL-AMP")
	{
		app_info ai;
		be_app->GetAppInfo (&ai);
		
		BEntry entry (&ai.ref);
		BPath path;
		entry.GetPath (&path);
		path.GetParent (&path);
		//path.Append ("data");
		path.Append ("scripts");
		
		if (firstWord == "/SOUNDPLAY")
			path.Append ("soundplay-hey");
		else
			path.Append ("cl-amp-clr");
		
		BMessage *msg (new BMessage);
		msg->AddString ("exec", path.Path());
		msg->AddPointer ("client", this);
		
		thread_id execThread = spawn_thread (
			ExecPipe,
			"exec_thread",
			B_LOW_PRIORITY,
			msg);

		resume_thread (execThread);
	
		return true;
	}
		


	if (firstWord == "/ABOUT")
	{
		be_app_messenger.SendMessage (B_ABOUT_REQUESTED);
		
		return true;
	}
	
	
	if (firstWord == "/AWAY")
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
		
		return true;	
	}
	
	
	if (firstWord == "/BACK")
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
		
		return true;
	}
	
	
	if (firstWord == "/CHANSERV"
	||  firstWord == "/NICKSERV"
	||  firstWord == "/MEMOSERV")
	{
		BString theCmd (firstWord.RemoveFirst ("/")),
				theRest (RestOfString (data, 2));
		theCmd.ToLower();
	
		if (theRest != "-9z99")
		{
			if (bowser_app->GetMessageOpenState())
			{
				BMessage msg (OPEN_MWINDOW);
				BMessage buffer (M_SUBMIT);
	
				buffer.AddString ("input", theRest.String());
				msg.AddMessage ("msg", &buffer);
				msg.AddString ("nick", theCmd.String());
				sMsgr.SendMessage (&msg);
			}
			else
			{
				BString tempString;
				
				tempString << "[M]-> " << theCmd << " > " << theRest << "\n";
				Display (tempString.String(), 0);
	
				BMessage send (M_SERVER_SEND);
				AddSend (&send, "PRIVMSG ");
				AddSend (&send, theCmd);
				AddSend (&send, " :");
				AddSend (&send, theRest);
				AddSend (&send, endl);
			}
		}
		
		return true;
	}
	
	
	if (firstWord == "/CLEAR")
	{
		text->ClearView (false);
		return true;
	}

	if (firstWord == "/FCLEAR")
	{
		text->ClearView (true);
		return true;
	}
	
	
	if (firstWord == "/CTCP")
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
	
			BMessage send (M_SERVER_SEND);
			AddSend (&send, "PRIVMSG ");
			AddSend (&send, theTarget << " :\1" << theAction << "\1");
			AddSend (&send, endl);
		}
		
		return true;
	}
	
	
	if (firstWord == "/DCC")
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
					sendFile.Unset();
					entry_ref ref;
					get_ref_for_path(sendPath.Path(), &ref);
					msg->AddRef("refs", &ref);
					sMsgr.SendMessage(msg);	
					return true;	
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
		
		return true;
	}
	
	
	if (firstWord == "/DOP" || firstWord == "/DEOP")
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
		
		return true;
	}
	
	
	if (firstWord == "/DESCRIBE")
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
		
		return true;
	}
		
	
	if (firstWord == "/DNS")
	{
		BString parms (GetWord(data, 2));
	
		ChannelWindow *window;
		MessageWindow *message;
		
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
						return true;				
					}
				}
		}
	
		else if ((message = dynamic_cast<MessageWindow *>(this)))
		{
			BString eid (id);
			eid.RemoveLast (" [DCC]");
			if (!ICompare(eid, parms) || !ICompare(myNick, parms))
			{
				BMessage send (M_SERVER_SEND);
				AddSend (&send, "USERHOST ");
				AddSend (&send, parms.String());
				AddSend (&send, endl);
				PostMessage(&send);
				return true;
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
				B_LOW_PRIORITY,
				msg);
	
			resume_thread (lookupThread);
		}
		
		return true;
	}
	
	
	if (firstWord == "/PEXEC") // piped exec
	{
		
		BString theCmd (RestOfString (data, 2));
		
		if (theCmd != "-9z99")
		{
			BMessage *msg (new BMessage);
			msg->AddString ("exec", theCmd.String());
			msg->AddPointer ("client", this);
			
			thread_id execThread = spawn_thread (
				ExecPipe,
				"exec_thread",
				B_LOW_PRIORITY,
				msg);
	
			resume_thread (execThread);
		
		}
		
		return true;
	
	}
	
	
	if (firstWord == "/EXCLUDE")
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
		
		return true;
	}
	
	
	if (firstWord == "/IGNORE")
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
		
		return true;
	}	
	
	if (firstWord == "/INVITE" || firstWord == "/I")
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
		
		return true;	
	}
	
	
	if (firstWord == "/JOIN" || firstWord == "/J")
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
		
		return true;
	}
	
	if (firstWord == "/KICK" || firstWord == "/K")
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
		
		return true;
	}
	
	
	if (firstWord == "/LIST")
	{
		BMessage msg (M_LIST_COMMAND);

		msg.AddString ("cmd", data);
		msg.AddString ("server", serverName.String());
		msg.AddRect ("frame", Frame());
		bowser_app->PostMessage (&msg);
	
		return true;
	}
	
	
	if (firstWord == "/M")
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
	
		return true;
	}
	
	
	if (firstWord == "/ME")
	{
		BString theAction (RestOfString (data, 2));

		if (theAction != "-9z99")
		{
			ActionMessage (theAction.String(), myNick.String());
		}
		
		return true;
	}

		
	if (firstWord == "/MODE")
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
		
		return true;
	}
	
	
	if (firstWord == "/MSG")
	{
		BString theRest (RestOfString (data, 3));
		BString theNick (GetWord (data, 2));
	
		if (theRest != "-9z99"
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
		return true;
	}	
		
	if (firstWord == "/NICK")
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
		
		return true;
	}
	
	
	if (firstWord == "/NOTICE")
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
		
		return true;
	}
	
	
	if (firstWord == "/NOTIFY")
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
		
		return true;
	}

	if (firstWord == "/OP")
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
		
		return true;
	}
	

	if (firstWord == "/PART")
	{
		BMessage msg (B_QUIT_REQUESTED);

		msg.AddBool ("bowser:part", true);
		PostMessage (&msg);

		return true;
	}	
	
	
	if (firstWord == "/PING")
	{
		BString theNick (GetWord (data, 2));
	
		if (theNick != "-9z99")
		{
			long theTime (time (0));
			BString tempString ("/CTCP ");
	
			tempString << theNick << " PING " << theTime;
			SlashParser (tempString.String());
		}
	
		return true;
	}
	
	
	if (firstWord == "/PREFERENCES")
	{
		be_app_messenger.SendMessage (M_PREFS_BUTTON);
		
		return true;
	}
	
	
	if (firstWord == "/QUERY" || firstWord == "/Q")
	{
		BString theNick (GetWord (data, 2));

		if (theNick != "-9z99")
		{
			BMessage msg (OPEN_MWINDOW);
	
			msg.AddString ("nick", theNick.String());
			sMsgr.SendMessage (&msg);
		}
	
		return true;	
	}

	
	if (firstWord == "/QUIT")
	{
		BString theRest (RestOfString (data, 2));
		BString buffer;
	
		if (theRest == "-9z99")
		{
			const char *expansions[1];
			BString version (bowser_app->BowserVersion());
	
			expansions[0] = version.String();
			theRest = ExpandKeyed (bowser_app
				->GetCommand (CMD_QUIT).String(), "V", expansions);
		}
	
		buffer << "QUIT :" << theRest;
	
		BMessage msg (B_QUIT_REQUESTED);
		msg.AddString ("bowser:quit", buffer.String());
		sMsgr.SendMessage (&msg);
	
		return true;
	}
	
	
	if (firstWord == "/RAW" || firstWord == "/QUOTE")
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
		
		return true;
	}
	
	
	if (firstWord == "/RECONNECT")
	{
		BMessage msg (M_SLASH_RECONNECT);
		msg.AddString ("server", serverName.String());
		bowser_app->PostMessage (&msg);
		return true;
	}
	
	
	if (firstWord == "/SLEEP")
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
		
		return true;
	
	}
	
		
	if (firstWord == "/TOPIC" || firstWord == "/T")
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
		
		return true;
	}
	
	
	if (firstWord == "/UNIGNORE")
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
	
		return true;
	}
	
	
	if (firstWord == "/UNNOTIFY")
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
	
		return true;
	}
	
	
	if (firstWord == "/UPTIME")
	{
		BString parms (GetWord(data, 2));
		
		BString uptime (DurationString(system_time()));
		BString expandedString;
		const char *expansions[1];
		expansions[0] = uptime.String();
		expandedString = ExpandKeyed (bowser_app->GetCommand (CMD_UPTIME).String(), "U",
			expansions);
		expandedString.RemoveFirst("\n");
	
		if ((id != serverName) && (parms == "-9z99"))
		{
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
			BString tempString;
				
			tempString << "Uptime: " << expandedString << "\n";
			Display (tempString.String(), &whoisColor);
			
		}
		
		return true;
	}
	
	
	if (firstWord == "/VERSION"
	||  firstWord == "/TIME")
	{
		BString theCmd (firstWord.RemoveFirst ("/")),
				theNick (GetWord (data, 2));
		theCmd.ToUpper();
	
		// the "." check is because the user might specify a server name
		
		if (theNick != "-9z99" && theNick.FindFirst(".") < 0)
		{
			BString tempString ("/CTCP ");
	
			tempString << theNick << " " << theCmd;
			SlashParser (tempString.String());
		}
		else
		{
		  	BMessage send (M_SERVER_SEND);
	
			AddSend (&send, theCmd);
			
			if (theNick != "-9z99")
			{
				AddSend (&send, " ");
				AddSend (&send, theNick);
			}
						
			AddSend (&send, endl);
		}
		
		return true;
	}
	
	
	if (firstWord == "/VISIT")
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
		
		return true;
	}



	if (firstWord != "" && firstWord[0] == '/')
	// != "" is required to prevent a nasty crash with string[0]
	{
		BString theCmd (firstWord.RemoveAll ("/")),
	            theRest (RestOfString (data, 2));
		
		BMessage send (M_SERVER_SEND);
		
		if (theCmd == "W")
			theCmd = "WHOIS";

		AddSend (&send, theCmd);
	
		if (theRest != "-9z99")
		{
			AddSend (&send, " ");
			AddSend (&send, theRest);
		}
		AddSend (&send, endl);	

		return true;
	}
	
	return false;  // we couldn't handle this message

}


int32
ClientWindow::ExecPipe (void *arg)
{
	BMessage *msg (reinterpret_cast<BMessage *>(arg));
	const char *exec;
	ClientWindow *client;
	
	msg->FindString ("exec", &exec);
	msg->FindPointer ("client", reinterpret_cast<void **>(&client));
	
	delete msg;
	
	FILE *fp = popen(exec, "r");

	char read[768]; // should be long enough for any line...
		
	while (fgets(read, 768, fp))
	{
		read[strlen(read)-1] = '\0'; // strip newline		
		
		BMessage msg (M_SUBMIT_RAW);
		msg.AddBool ("lines", 1);
		msg.AddString ("data", read);			
		client->PostMessage (&msg);
	}
		
	pclose(fp);
	
	return 0;

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
	
	if (isalpha(resolve[0]))
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
