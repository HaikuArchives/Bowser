
// This code derives from a lot of sources; most notably, ircII,
// protoIRC and Baxter. Big thanks to all.

#include <Alert.h>

#include <FilePanel.h>
#include <MessageFilter.h>

#include <Path.h>

#include <stdio.h>

#include "Bowser.h"
#include <TextControl.h>
#include "ServerWindow.h"

class DCCFileFilter : public BMessageFilter
{
	BFilePanel					*panel;
	BMessage						send_msg;

	public:

									DCCFileFilter (BFilePanel *, const BMessage &);
	virtual						~DCCFileFilter (void);

	virtual filter_result	Filter (BMessage *, BHandler **);
	filter_result				HandleButton (BMessage *);
	filter_result				HandleAlert (BMessage *);
};

const uint32 M_FILE_PANEL_BUTTON				= 'Tact';
const uint32 M_FILE_PANEL_ALERT				= 'alrt';

void
ServerWindow::DCCGetDialog (
	BString nick,
	BString file,
	BString size,
	BString ip,
	BString port)
{
	BMessage msg (DCC_ACCEPT), reply;
	
	msg.AddString ("bowser:nick", nick.String());
	msg.AddString ("bowser:file", file.String());
	msg.AddString ("bowser:size", size.String());
	msg.AddString ("bowser:ip", ip.String());
	msg.AddString ("bowser:port", port.String());


	//bool handled (false);

// ignore this part until some minor details with DCC Prefs are worked out
/*
		const char *directory = "/boot/home/";
		entry_ref ref;
		BEntry entry;

		create_directory (directory, 0777);
		if (entry.SetTo (directory) == B_NO_ERROR 
		if (entry.GetRef (&ref)     == B_NO_ERROR)
		{
			BDirectory dir (&ref);
			BEntry file_entry; 
			struct stat s;

			if (file_entry.SetTo (&dir, file.String()) == B_NO_ERROR
			&&  file_entry.GetStat (&s)       == B_NO_ERROR
			&&  S_ISREG (s.st_mode))
			{
				BString buffer;
				BAlert *alert;
				int32 which;

				buffer << "The file \""
					<< file
					<< "\" already exists in the specified folder.  "
						"Do you want to continue the transfer?";

				alert = new BAlert (
					"DCC Request",
					buffer.String(),
					"Refuse",
					"Get",
					"Resume",
					B_WIDTH_AS_USUAL,
					B_OFFSET_SPACING,
					B_WARNING_ALERT);

				which = alert->Go();

				if (which == 0)
				{
					return;
				}

				if (which == 2)
				{
					BFile file (&file_entry, B_READ_ONLY);
					off_t position;
					BPath path;

					file.GetSize (&position);
					file_entry.GetPath (&path);
					msg.AddString ("path", path.Path());
					msg.AddInt64 ("pos", position);

					AddResumeData (&msg);
					return;
				}
			}

			msg.AddRef ("directory", &ref);
			msg.AddString ("name", file);
			sMsgr.SendMessage (&msg);
			handled = true;
		}
	}
*/
	BFilePanel *panel;
	BString text;

	text << nick
		<< ": "
		<< file
		<< " ("
		<< size
		<< " bytes)";

	panel = new BFilePanel (
		B_SAVE_PANEL,
		&sMsgr,
		0,
		0,
		false,
		&msg);
	panel->SetButtonLabel (B_DEFAULT_BUTTON, "Accept");
	panel->SetButtonLabel (B_CANCEL_BUTTON, "Refuse");
	panel->SetSaveText (file.String());

	if (panel->Window()->Lock())
	{
		panel->Window()->SetTitle (text.String());
		panel->Window()->AddFilter (new DCCFileFilter (panel, msg));
		panel->Window()->Unlock();
	}
	panel->Show();
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

DCCFileFilter::DCCFileFilter (BFilePanel *p, const BMessage &msg)
	: BMessageFilter (B_ANY_DELIVERY, B_ANY_SOURCE),
	  panel (p),
	  send_msg (msg)
{
}

DCCFileFilter::~DCCFileFilter (void)
{
}

filter_result
DCCFileFilter::Filter (BMessage *msg, BHandler **)
{
	filter_result result (B_DISPATCH_MESSAGE);
	
	switch (msg->what)
	{
		case M_FILE_PANEL_BUTTON:
		{
			result = HandleButton (msg);
			break;
		}

		case M_FILE_PANEL_ALERT:
		{
			result = HandleAlert (msg);
			break;
		}

		case B_QUIT_REQUESTED:
		{
			printf ("Panel Quit Requested\n");
			break;
		}
	}

	return result;
}

filter_result
DCCFileFilter::HandleButton (BMessage *)
{
	filter_result result (B_DISPATCH_MESSAGE);
	BTextControl *text (dynamic_cast<BTextControl *>(
		panel->Window()->FindView ("text view")));

	if (text)
	{
		BDirectory dir;
		struct stat s;
		entry_ref ref;
		BEntry entry;

		panel->GetPanelDirectory (&ref);

		dir.SetTo (&ref);
		
		if (entry.SetTo (&dir, text->Text()) == B_NO_ERROR
		&&  entry.GetStat (&s)               == B_NO_ERROR
		&&  S_ISREG (s.st_mode))
		{
			BString buffer;
			BAlert *alert;

			buffer << "The file \""
				<< text->Text()
				<< "\" already exists in the specified folder.  "
					"Do you want to continue the transfer?";

			alert = new BAlert (
				"DCC Request",
				buffer.String(),
				"Cancel",
				"Get",
				"Resume",
				B_WIDTH_AS_USUAL,
				B_OFFSET_SPACING,
				B_WARNING_ALERT);

			alert->Go (new BInvoker (
				new BMessage (M_FILE_PANEL_ALERT),
				panel->Window()));

			result = B_SKIP_MESSAGE;
		}
	}
	return result;
}

filter_result
DCCFileFilter::HandleAlert (BMessage *msg)
{
	BTextControl *text (dynamic_cast<BTextControl *>(
		panel->Window()->FindView ("text view")));
	int32 which;

	msg->FindInt32 ("which", &which);

	if (which == 0 || text == 0)
	{
		return B_SKIP_MESSAGE;
	}

	entry_ref ref;
	panel->GetPanelDirectory (&ref);

	if (which == 2)
	{
		BDirectory dir (&ref);
		BFile file (&dir, text->Text(), B_READ_ONLY);
		BEntry entry (&dir, text->Text());
		BPath path;
		off_t position;

		file.GetSize (&position);
		entry.GetPath (&path);
		send_msg.AddString ("path", path.Path());
		send_msg.AddInt64  ("pos", position);

		send_msg.what = M_ADD_RESUME_DATA;
	}
	else
	{
		send_msg.AddRef ("directory", &ref);
		send_msg.AddString ("name", text->Text());
	}

	panel->Messenger().SendMessage (&send_msg);

	BMessage cmsg (B_CANCEL);
	cmsg.AddPointer ("source", panel);
	panel->Messenger().SendMessage (&cmsg);

	return B_SKIP_MESSAGE;
}
