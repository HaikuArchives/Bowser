
// This code derives from a lot of sources; most notably, ircII,
// protoIRC and Baxter. Big thanks to all.

#include <Alert.h>
#include <FilePanel.h>

#include "IRCDefines.h"
#include "ServerWindow.h"

void
ServerWindow::DCCGetDialog (
	BString theNick,
	BString theFile,
	BString theSize,
	BString theIP,
	BString thePort)
{
	BString theText (theNick);
	theText << ": " << theFile << " (" << theSize << " bytes)";

	BMessage msg (DCC_ACCEPT);
	msg.AddString ("bowser:nick", theNick.String());
	msg.AddString ("bowser:file", theFile.String());
	msg.AddString ("bowser:size", theSize.String());
	msg.AddString ("bowser:ip", theIP.String());
	msg.AddString ("bowser:port", thePort.String());

	BFilePanel *fPanel (new BFilePanel (
		B_SAVE_PANEL,
		&sMsgr,
		0,
		0,
		false,
		&msg));

	fPanel->SetButtonLabel (B_DEFAULT_BUTTON, "Accept");
	fPanel->SetButtonLabel (B_CANCEL_BUTTON, "Refuse");
	fPanel->SetSaveText (theFile.String());
	fPanel->Window()->SetTitle (theText.String());
	fPanel->Show();
}

void ServerWindow::DCCChatDialog(BString theNick, BString theIP, BString thePort)
{
	BString theText(theNick);
	theText << " wants to begin a DCC chat with you.";
	BAlert *myAlert = new BAlert("DCC Request", theText.String(), "Accept",
		"Refuse");
	BMessage *myMessage = new BMessage(CHAT_ACCEPT);
	myMessage->AddString("nick", theNick.String());
	myMessage->AddString("ip", theIP.String());
	myMessage->AddString("port", thePort.String());
	BInvoker *myInvoker = new BInvoker(myMessage, this);
	myAlert->Go(myInvoker);
}

