
#include <View.h>
#include <CheckBox.h>
#include <TextView.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Application.h>

#include <stdio.h>

#include "IRCDefines.h"
#include "SetupWindow.h"
#include "Settings.h"
#include "ServerOptions.h"

ServerOptions::ServerOptions (
	BRect frame,
	ServerData *sd,
	const BMessenger &sumsgr)
	: BWindow (
		frame,
		"Options:",
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE),

		serverData (sd),
		suMsgr (sumsgr)
{
	settings = new WindowSettings (
		this,
		"soptions",
		BOWSER_SETTINGS);
	settings->Restore();

	frame = Bounds();

	BView *bgView (new BView (
		frame,
		"background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW));
	bgView->SetViewColor (222, 222, 222, 255);
	AddChild (bgView);

	connectBox = new BCheckBox (
		BRect (
			frame.left + 10,
			frame.top + 10,
			frame.right - 10,
			frame.bottom ),
		"connect",
		"Connect on startup",
		new BMessage(M_SOPTIONS_CONNECT_ON_STARTUP));
	connectBox->ResizeToPreferred();
	connectBox->SetValue (serverData->connect ? B_CONTROL_ON : B_CONTROL_OFF);
	bgView->AddChild (connectBox);
	
	motdBox = new BCheckBox (
		BRect (
			frame.left + 10,
			connectBox->Frame().bottom + 1,
			frame.right - 10,
			frame.bottom),
		"motd",
		"Show message of the day",
		new BMessage (M_SOPTIONS_MOTD));
	motdBox->ResizeToPreferred();
	motdBox->SetValue (serverData->motd ? B_CONTROL_ON : B_CONTROL_OFF);
	bgView->AddChild (motdBox);

	identdBox = new BCheckBox (
		BRect (
			frame.left + 10,
			motdBox->Frame().bottom + 1,
			frame.right - 10,
			frame.bottom),
		"identd",
		"Enable built-in ident server",
		new BMessage (M_SOPTIONS_IDENTD));
	identdBox->ResizeToPreferred();
	identdBox->SetValue (serverData->identd ? B_CONTROL_ON : B_CONTROL_OFF);
	bgView->AddChild (identdBox);

	BStringView *startLabel (new BStringView (
		BRect (
			frame.left + 10,
			identdBox->Frame().bottom + 10,
			frame.right - 10,
			frame.bottom),
		"startLabel",
		"Connect Commands"));
	startLabel->SetFont (be_bold_font);
	startLabel->ResizeToPreferred();
	bgView->AddChild (startLabel);

	width = motdBox->Frame().Width();

	if (identdBox->Frame().Width() > width)
		width = identdBox->Frame().Width();
	if (startLabel->Frame().Width() > width)
		width = startLabel->Frame().Width();
	if (connectBox->Frame().Width() > width)
		width = connectBox->Frame().Width();
	ResizeTo (width + 20, Frame().Height());
	frame = Bounds();
		
	frame.left   += 10;
	frame.right  -= 10 + B_V_SCROLL_BAR_WIDTH;
	frame.top     = startLabel->Frame().bottom + 1;
	frame.bottom  = startLabel->Frame().bottom + 100;

	connectText = new BTextView (
		BRect (frame),
		"connectText",
		frame.OffsetToCopy (B_ORIGIN),
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	connectText->SetText (serverData->connectCmds.String());
			
	scroller = new BScrollView (
		"scroller",
		connectText,
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0,
		false,
		true);
	bgView->AddChild (scroller);

	BString title ("Options: ");
	title << serverData->server;
	SetTitle (title.String());

	settings = new WindowSettings (
		this,
		"soptions",
		BOWSER_SETTINGS);
}

ServerOptions::~ServerOptions (void)
{
	delete settings;
}

bool
ServerOptions::QuitRequested (void)
{
	settings->Save();

	serverData->connectCmds = connectText->Text();
	BMessage msg (M_SOPTIONS_SHUTDOWN);

	msg.AddPointer ("server", serverData);
	suMsgr.SendMessage (&msg);

	return true;
}

void
ServerOptions::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_SOPTIONS_MOTD:
			serverData->motd = motdBox->Value() == B_CONTROL_ON;
			break;

		case M_SOPTIONS_IDENTD:
			serverData->identd = identdBox->Value() == B_CONTROL_ON;
			break;

		case M_SOPTIONS_CONNECT_ON_STARTUP:
			serverData->connect = connectBox->Value() == B_CONTROL_ON;
			break;
			
		default:
			BWindow::MessageReceived (msg);
	}
}

void
ServerOptions::Hide (void)
{
	settings->Save();
	BWindow::Hide();
}

void
ServerOptions::Show (void)
{
	settings->Restore();
	ResizeTo (width + 20, scroller->Frame().bottom + 10);
	BWindow::Show();
}
