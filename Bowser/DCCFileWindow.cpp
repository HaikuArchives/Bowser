
#include <File.h>
#include <Path.h>
#include <StatusBar.h>

#include <stdio.h>
#include <stdlib.h>
#include <net/socket.h>
#include <net/netdb.h>

#include "IRCDefines.h"
#include "Bowser.h"
#include "ServerWindow.h"
#include "DCCFileWindow.h"

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// DCC FILE WINDOW STUFF ///////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DCCFileWindow::DCCFileWindow (
	BString theType,
	BString theNick,
	BString theFile,
	BString theSize,
	BString theIP,
	BString thePort,
	ServerWindow *theCaller)
	: BWindow (
		BRect (115,115,415,155),
		"DCC",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE|B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	dNick = theNick;
	dIP = theIP;
	dPort = thePort;
	dSize = theSize;
	dFile = theFile;
	dType = theType;
	caller = theCaller;

	whoisColor = bowser_app->GetColor (C_WHOIS);

	BView *bgView = new BView(Bounds(), "Background", B_FOLLOW_ALL, B_WILL_DRAW);
	bgView->SetViewColor(222, 222, 222, 255);
	AddChild(bgView);

	BRect r(5,5,295,40);

	char tempString[512];	
	float theK = atol(theSize.String()) / 1024.0;
	sprintf(tempString, " / %.1fk", theK);
	BString receiveTemp;

	if(theType == "GET")
	{
		BPath path (theFile.String());

		receiveTemp = "Receiving: ";
		receiveTemp << path.Leaf();	
	}
	else if(theType == "SEND")
	{
		receiveTemp = "Sending: ";
		BPath myPath(dFile.String(), NULL, false);
		receiveTemp << myPath.Leaf();
	}
	SetTitle(receiveTemp.String());

	myStatus = new BStatusBar(r, "status", "bps: ", tempString);
	myStatus->SetMaxValue(atol(theSize.String()));
	myStatus->SetBarColor(whoisColor);
	bgView->AddChild(myStatus);

	Show();

	if(theType == "GET")
	{
		dataThread = spawn_thread(dcc_receive, "DCC Receive", B_NORMAL_PRIORITY, this);
		resume_thread(dataThread);
	}
	else if(theType == "SEND")
	{
		dataThread = spawn_thread(dcc_send, "DCC Send", B_NORMAL_PRIORITY, this);
		resume_thread(dataThread);
	}
}

DCCFileWindow::~DCCFileWindow()
{
}

void DCCFileWindow::Quit()
{
	suspend_thread(dataThread);
	kill_thread(dataThread);
	snooze(1000);
	BWindow::Quit();
}

bool DCCFileWindow::QuitRequested()
{
	return true;
}

void DCCFileWindow::MessageReceived(BMessage *message)
{
	if(message == NULL)
		return;

	switch(message->what)
	{
		case M_STATE_CHANGE:

			if (message->HasData ("color", B_RGB_COLOR_TYPE))
			{
				const rgb_color *color;
				int32 which;
				ssize_t size;

				message->FindInt32 ("which", &which);
				message->FindData (
					"color",
					B_RGB_COLOR_TYPE,
					reinterpret_cast<const void **>(&color),
					&size);

				if (which == C_WHOIS)
					whoisColor = *color;

			}
			break;

		default:
			BWindow::MessageReceived (message);
	}
}

int32 DCCFileWindow::dcc_receive(void *arg)
{
	DCCFileWindow *obj = (DCCFileWindow *)arg;
	return (obj->DCCReceive());
}

int32 DCCFileWindow::dcc_send(void *arg)
{
	DCCFileWindow *obj = (DCCFileWindow *)arg;
	return (obj->DCCSend());
}

int32 DCCFileWindow::DCCReceive()
{
	struct sockaddr_in sa;
	int status;
	char tempIP[50];
	sprintf(tempIP, "%s", dIP.String());
	char *endpoint;
	u_long realIP = strtoul(tempIP, &endpoint, 10);
	
	if((mySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		SetTitle("Error opening socket");
		printf("Error opening socket.\n");
		return false;
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons(atoi(dPort.String()));
	sa.sin_addr.s_addr = htonl(realIP);
	memset(sa.sin_zero, 0, sizeof(sa.sin_zero));

	status = connect(mySocket, (struct sockaddr *)&sa, sizeof(sa));
	if(status < 0)
	{
		SetTitle("Error connecting socket");
		printf("Error connecting socket.\n");
		closesocket(mySocket);
		return false;
	}

	BFile *myFile = new BFile(dFile.String(), B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	char incomingBuffer[BIG_ENOUGH_FOR_A_REALLY_FAST_ETHERNET];
	long bytesReceived = 0;
	realSize = atol(dSize.String());
	int lengthRead;
	long sizeToRead = sizeof(incomingBuffer);
	u_long feedbackSize = 0;
	
	if(sizeToRead > realSize)
	{
		sizeToRead = realSize;
	}

	bigtime_t last (system_time()), now;
	int cps (0), period (0);

	while (bytesReceived < realSize 
	&&    (lengthRead = recv (mySocket, incomingBuffer, sizeToRead, 0)))
	{
		if(lengthRead < 0)
		{
			SetTitle("Transfer aborted");
			closesocket(mySocket);
			delete myFile;
			return 0;
		}

		myFile->Write (incomingBuffer, lengthRead);

		bytesReceived += lengthRead;
		feedbackSize = htonl(bytesReceived);
		send(mySocket, &feedbackSize, 4, 0); // confirm receipt of data

		now = system_time();
		period += lengthRead;
		bool hit (false);

		if (now - last > 500000)
		{
			cps = (int)ceil(period / ((now - last) / 1000000.0));
			last = now;
			period = 0;
			hit = true;
		}

		UpdateWindow (lengthRead, cps, bytesReceived, hit);
	}

	BString tempString("Done receiving ");
	tempString << dFile;
	SetTitle(tempString.String());
	closesocket(mySocket);
	delete myFile;

	PostMessage (B_QUIT_REQUESTED);

	return 0;
}

int32 DCCFileWindow::DCCSend()
{
	int32 myPort = 1500 + (rand() % 5000); // baxter's way of getting a port
	struct sockaddr_in sa, socky;
	
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if(mySocket < 0)
	{
		SetTitle("Error creating socket");
		printf("Error creating socket.\n");
		return -1;
	}
	
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = caller->LocalAddress();
	sa.sin_port = htons(myPort);
	if(bind(mySocket, (struct sockaddr*)&sa, sizeof(sa)) == -1)
	{
		myPort = 1500 + (rand() % 5000); // try once more
		sa.sin_port = htons(myPort);
		if(bind(mySocket, (struct sockaddr*)&sa, sizeof(sa)) == -1)
		{
			SetTitle("Error binding socket");
			printf("Error binding socket.\n");
			return -1;
		}
	}

	int tempSize = sizeof(socky);
	getsockname(mySocket, (struct sockaddr*)&socky, &tempSize);

	BFile *myFile = new BFile(dFile.String(), B_READ_ONLY);
	if(myFile->InitCheck() != B_OK)
	{
		SetTitle("Error finding file");
		printf("File not found!\n");
		delete myFile;
		return -1;
	}

	BPath myPath(dFile.String(), NULL, false);
	BString theFile(myPath.Leaf());
	theFile.ReplaceAll(" ", "_");
	BString sendString("PRIVMSG ");
	sendString << dNick << " :\1DCC SEND " << theFile << ' ';
	sendString << htonl(socky.sin_addr.s_addr)
		<< ' ' << myPort << ' ' << dSize << '\1';

	BMessage msg (M_SERVER_SEND);
	msg.AddString ("data", sendString.String());
	caller->PostMessage (&msg);
	
	listen(mySocket, 1);
	struct sockaddr_in remoteAddy;
	int theLen = sizeof(remoteAddy);
	int acceptSocket = accept(mySocket, (struct sockaddr*)&remoteAddy, &theLen);

	int32 myLength = 0, sendInPass = 0, bytesSent = 0;
	int32 fileSize = atol(dSize.String());
	char myBuffer[1024];
	
	bigtime_t last (system_time()), now;
	int cps (0), period (0);

	while (fileSize > bytesSent
	&&    (myLength = myFile->Read(myBuffer, sizeof(myBuffer))))
	{
		sendInPass = send (acceptSocket, myBuffer, myLength, 0);
		bytesSent += sendInPass;

		uint32 confirm;
		recv (acceptSocket, &confirm, sizeof (confirm), 0);
		
		now = system_time();
		period += sendInPass;
		bool hit (false);

		if (now - last > 500000)
		{
			cps = (int)ceil (period / ((now - last) / 1000000.0));
			last = now;
			period = 0;
			hit = true;
		}

		UpdateWindow (sendInPass, cps, bytesSent, hit);
	}

	delete myFile;
	closesocket(mySocket);
	closesocket(acceptSocket);
	
	BString tempString("Done sending ");
	tempString << theFile;
	SetTitle(tempString.String());

	PostMessage (B_QUIT_REQUESTED);

	return 0;
}

void DCCFileWindow::UpdateWindow (
	int lengthRead,
	int cps,
	long bytesReceived,
	bool labelUpdate)
{
	if(Lock())
	{
		if (labelUpdate)
		{
			char leading[64];
			char trailing[64];

			sprintf (trailing, "%.1f", bytesReceived / 1024.0);
			sprintf (leading,  "%d", cps);

			myStatus->Update (lengthRead, leading, trailing);
		}
		else
			myStatus->Update (lengthRead);
		Unlock();
	}
}
