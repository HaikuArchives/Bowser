
#include <Application.h>

#include <stdlib.h>
#include <stdio.h>

#include "Bowser.h"
#include "StringManip.h"
#include "ServerWindow.h"

void ServerWindow::ParseCTCP(BString theNick, BString theMsg)
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
	else if(theCTCP == "VERSION")
	{
		BString sysInfoString;
		if(!bowser_app->GetParanoidState())
		{
			system_info myInfo;
			get_system_info(&myInfo);
			sysInfoString = "BeOS";
			if(myInfo.platform_type == B_AT_CLONE_PLATFORM)
				sysInfoString << "/Intel : ";
			else if(myInfo.platform_type == B_MAC_PLATFORM)
				sysInfoString << "/PPC : ";
			else
				sysInfoString << "/BeBox : ";
			sysInfoString << myInfo.cpu_count << " CPU(s) @ ~"
				<< myInfo.cpu_clock_speed / 1000000 << "MHz";
		}
		else
		{
			sysInfoString = "BeOS : don't think twice.";
		}
		BString tempString("NOTICE ");
		tempString << theNick << " :\1VERSION Bowser[d" 
			<< VERSION << "] : " << sysInfoString;
		#ifdef DEV_BUILD
		tempString << " (development build)";
		#endif
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

void ServerWindow::ParseCTCPResponse(BString theNick, BString theMsg)
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
