
#include <Application.h>
#include <AppFileInfo.h>

#include <stdlib.h>
#include <stdio.h>

#include "Bowser.h"
#include "StringManip.h"
#include "ServerWindow.h"

void
ServerWindow::ParseCTCP(BString theNick, BString theMsg)
{
	BString theCTCP = GetWord(theMsg.String(), 1).ToUpper();
	BString theRest = RestOfString(theMsg.String(), 2);
	theCTCP.RemoveFirst("\1");
	theCTCP.RemoveLast("\1");

	if(theCTCP == "PING")
	{
		if(theMsg == "-9z99")
			return;
		BString tempString("NOTICE ");
		tempString << theNick << " :" << theMsg;
		SendData (tempString.String());
	}
	else if((theCTCP == "VERSION") || (theCTCP == "CLIENTINFO"))
	{
		BString sysInfoString;
		if(!bowser_app->GetParanoidState())
		{
			BString librootversion;
			BFile *libroot = new BFile("/boot/beos/system/lib/libroot.so", B_READ_ONLY);
			BAppFileInfo info(libroot);
			version_info version;
			info.GetVersionInfo(&version, B_SYSTEM_VERSION_KIND);
			librootversion = version.short_info;
			librootversion.RemoveFirst ("R");
			
			delete libroot;
						
			system_info myInfo;
			get_system_info(&myInfo);
			
			sysInfoString = "BeOS/";
			sysInfoString << librootversion;
			
			// this is the way the BeOS 5.0.1 update checks for R5 Pro...
			bool BePro;
			BePro = true; // innocent until proven guilty
			BFile *indeo5rt = new BFile("/boot/beos/system/add-ons/media/encoders/indeo5rt.encoder", B_READ_ONLY);
			BFile *indeo5rtmmx = new BFile("/boot/beos/system/add-ons/media/encoders/indeo5rtmmx.encoder", B_READ_ONLY);
			BFile *mp3 = new BFile("/boot/beos/system/add-ons/media/encoders/mp3.encoder", B_READ_ONLY);
			
			if ((indeo5rt->InitCheck() != B_OK) ||
				(indeo5rtmmx->InitCheck() != B_OK) ||
				(mp3->InitCheck() != B_OK))
			{
				BePro = false; // *gasp*! leeches!
			}
			
			delete indeo5rt;
			delete indeo5rtmmx;
			delete mp3;
				
			if (BePro)
				sysInfoString << " Pro Edition : ";
			else
				sysInfoString << " Personal Ed. : ";
						
			sysInfoString << myInfo.cpu_count << " CPU";
			if (myInfo.cpu_count > 1)
				sysInfoString << "s";
			
			sysInfoString << " @ ~" << myInfo.cpu_clock_speed / 1000000 << "MHz";
					
		}
		else
		{
			sysInfoString = "BeOS : Because you don't eat cereal with a fork.";
		}
		BString tempString("NOTICE ");
		tempString << theNick << " :\1VERSION Bowser[" 
			<< bowser_app->BowserVersion() << "] : http://bowser.sourceforge.net : " << sysInfoString;
			
//		#ifdef DEV_BUILD
//			tempString << " (development build)";
//		#endif
			
		tempString << "\1";
		SendData (tempString.String());
	}
	
	else if(theCTCP == "UPTIME")
	{
		
		BString uptime (DurationString(system_time()));
		BString expandedString;
		
		const char *expansions[1];
		expansions[0] = uptime.String();

		expandedString = ExpandKeyed (bowser_app->GetCommand (CMD_UPTIME).String(), "U",
		expansions);
		expandedString.RemoveFirst("\n");
		
		BString tempString("NOTICE ");
		tempString << theNick << " :\1UPTIME " 
			<< expandedString;
		tempString << '\1';
		SendData (tempString.String());

	}
	else if(theCTCP == "DCC")
	{
		BString theType = GetWord(theMsg.String(), 2);
		if(theType == "SEND")
		{
			BString theFile = GetWord(theMsg.String(), 3);
			BString theIP = GetWord(theMsg.String(), 4);
			BString thePort = GetWord(theMsg.String(), 5);
			BString theSize = GetWord(theMsg.String(), 6);
			theSize.RemoveLast("\1"); // strip CTCP char
			DCCGetDialog(theNick, theFile, theSize, theIP, thePort);
		}
		else if(theType == "CHAT")
		{
			BString theIP = GetWord(theMsg.String(), 4);
			BString thePort = GetWord(theMsg.String(), 5);
			thePort.RemoveLast("\1");
			DCCChatDialog(theNick, theIP, thePort);
		}
		else if (theType == "ACCEPT")
		{
			BString file (GetWord (theMsg.String(), 3));
			BString port (GetWord (theMsg.String(), 4));
			BString poss (GetWord (theMsg.String(), 5));
			poss.RemoveLast("\1");

			off_t pos (0LL);
			for (int32 i = 0; i < poss.Length(); ++i)
				pos = pos * 10 + poss[i] - '0';

			for (int32 i = 0; i < resumes.CountItems(); ++i)
			{
				ResumeData *data ((ResumeData *)resumes.ItemAt (i));

				if (data->nick == theNick
				&&  data->pos  == pos
				&&  data->port == port);
				{
					resumes.RemoveItem (i);

					BMessage msg (DCC_ACCEPT);
					msg.AddString ("bowser:nick", data->nick.String());
					msg.AddString ("bowser:file", data->file.String());
					msg.AddString ("bowser:size", data->size.String());
					msg.AddString ("bowser:ip",   data->ip.String());
					msg.AddString ("bowser:port", data->port.String());
					msg.AddString ("path",        data->path.String());
					msg.AddBool   ("continue",    true);

					PostMessage(&msg);
					
					delete data;
					break;
				}
			}
		}
		else if (theType == "RESUME")
		{
			BString file (GetWord (theMsg.String(), 3));
			BString port (GetWord (theMsg.String(), 4));
			BString poss (GetWord (theMsg.String(), 5));
			poss.RemoveLast("\1");

			off_t pos (0LL);
			for (int32 i = 0; i < poss.Length(); ++i)
				pos = pos * 10 + poss[i] - '0';

			// Have to tell the sender we can resume
			BString tempString("PRIVMSG ");
			tempString << theNick << " :\1DCC ACCEPT " << file << " "
			           << port << " " << poss << "\1";
			SendData (tempString.String());

			BMessage bMsg (M_DCC_MESSENGER), bReply;
			be_app_messenger.SendMessage (&bMsg, &bReply);

			BMessenger msgr;
			bReply.FindMessenger ("msgr", &msgr);

			BMessage msg (M_ADD_RESUME_DATA), reply;
			msg.AddString ("bowser:nick", theNick.String());
			msg.AddString ("bowser:port", port.String());
			msg.AddString ("bowser:file", file.String());
			msg.AddInt64 ("bowser:pos", pos);

			// do sync.. we do not want to have the transfer
			// start before we tell it okay
			msgr.SendMessage (&msg, &reply);
			if (reply.HasBool ("hit")
			&&  reply.FindBool ("hit"))
			{

				BString buffer;

				buffer << "PRIVMSG "
					<< theNick
					<< " :\1DCC ACCEPT "
					<< file
					<< " "
					<< port
					<< " "
					<< poss
					<< "\1";
				SendData (buffer.String());
			}
		}
	}
	

	BMessage display (M_DISPLAY);
	BString buffer;

	buffer << "[" << theNick << " " << theCTCP;
	
	if (theCTCP == "PING" || theRest == "-9z99")
	{
		buffer << "]\n";
	}
	else
	{
		int32 theChars = theRest.Length();
		if (theRest[theChars - 1] == '\1')
		{
			theRest.Truncate(theChars - 1, false);
		}
		buffer << "] " << theRest << '\n';
	}
		
	PackDisplay (&display, buffer.String(), &ctcpReqColor, &serverFont);
	PostActive (&display);
}

void
ServerWindow::ParseCTCPResponse(BString theNick, BString theMsg)
{
	BString theResponse(theMsg);
	if(theResponse[0] == '\1')
		theResponse.Remove(0, 1);
	int32 theChars = theResponse.Length();
	if(theResponse[theChars - 1] == '\1')
		theResponse.Truncate(theChars - 1, false);

	BString firstWord = GetWord(theResponse.String(), 1).ToUpper();
	BString tempString;

	if(firstWord == "PING")
	{
		long curTime = time(NULL);
		long theSeconds = curTime - atoi(GetWord(theMsg.String(), 2).String());

		if(theSeconds > 10000) // catch possible conversion error(s)
		{
			theSeconds = curTime - atoi(GetWord(theMsg.String(), 2).String());
			if(theSeconds > 10000)
			{
				theSeconds = curTime - atoi(GetWord(theMsg.String(), 2).String());
				if(theSeconds > 10000)
				{
					theSeconds = curTime - atoi(GetWord(theMsg.String(), 2).String());
				}
			}
		}
		tempString << "[" << theNick << " PING response]: ";
		if(theSeconds != 1)
			tempString << theSeconds << " seconds\n";
		else
			tempString << "1 second\n";
	}
	else
	{
		BString theReply = RestOfString(theResponse.String(), 2);
		tempString << "[" << theNick << " " << firstWord << " response]: ";
		tempString << theReply << '\n';
	}
	
	BMessage display (M_DISPLAY);
	BString buffer;

	buffer << tempString.String();
	PackDisplay (&display, buffer.String(), &ctcpRpyColor, &serverFont);
	PostActive (&display);
}