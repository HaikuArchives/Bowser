
#include <CheckBox.h>

#include "Bowser.h"
#include "PrefWindow.h"

PreferenceWindow::PreferenceWindow (void)
	: BView (
		BRect (0, 0, 270, 150),
	  "PrefWindow",
	  B_FOLLOW_LEFT | B_FOLLOW_TOP,
	  B_WILL_DRAW)
{
	BRect bounds (Bounds());

	messageBox = new BCheckBox (
		BRect (0, 0, bounds.right, 24),
		"message",
		"Message command opens new query window",
		new BMessage (M_MESSAGE_OPEN));
	messageBox->SetValue (bowser_app->GetMessageOpenState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (messageBox);

	windowFollows = new BCheckBox (
		BRect (0, 25, bounds.right, 49),
		"windowfollow",
		"Window selection follows workspace",
		new BMessage (M_WINDOW_FOLLOWS));
	windowFollows->SetValue (bowser_app->GetWindowFollowsState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (windowFollows);
	
	hideSetup = new BCheckBox (
		BRect (0, 50, bounds.right, 74),
		"hidesetup",
		"Hide setup window on connect",
		new BMessage (M_HIDE_SETUP));
	hideSetup->SetValue (bowser_app->GetHideSetupState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (hideSetup);

	showSetup = new BCheckBox (
		BRect (0, 75, bounds.right, 99),
		"showsetup",
		"Activate setup window on disconnect",
		new BMessage (M_ACTIVATE_SETUP));
	showSetup->SetValue (bowser_app->GetShowSetupState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showSetup);
	
	showTopic = new BCheckBox (
		BRect (0, 125, bounds.right, 149),
		"showtopic",
		"Show channel topic in Titlebar",
		new BMessage (M_SHOW_TOPIC));
	showTopic->SetValue (bowser_app->GetShowTopicState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showTopic);

//	SetupAltW = new BCheckBox (
//		BRect (0, 150, bounds.right, 174),
//		"setupaltw",
//		"Enable AltW for Setup Window",
//		new BMessage (M_SHOW_TOPIC));
//	SetupAltW->SetValue (bowser_app->GetShowTopicState()
//		? B_CONTROL_ON : B_CONTROL_OFF);
//	AddChild (SetupAltW);
//
//	altwServer = new BCheckBox (
//		BRect (0, 175, bounds.right, 199),
//		"altwserver",
//		"Enable AltW for Server Windows",
//		new BMessage (M_SHOW_TOPIC));
//	altwServer->SetValue (bowser_app->GetShowTopicState()
//		? B_CONTROL_ON : B_CONTROL_OFF);
//	AddChild (altwServer);

}



PreferenceWindow::~PreferenceWindow (void)
{
}

void
PreferenceWindow::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	messageBox->SetTarget (this);
	windowFollows->SetTarget (this);
	hideSetup->SetTarget (this);
	showSetup->SetTarget (this);
	showTopic->SetTarget (this);
//	altwSetup->SetTarget (this);
//	altwServer->SetTarget (this);

	messageBox->ResizeToPreferred();
	windowFollows->ResizeToPreferred();
	hideSetup->ResizeToPreferred();
	showSetup->ResizeToPreferred();
	showTopic->ResizeToPreferred();
//	altwSetup->ResizeToPreferred();
//	altwServer->ResizeToPreferred();

	windowFollows->MoveTo (0, messageBox->Frame().bottom + 1);
	hideSetup->MoveTo (0, windowFollows->Frame().bottom + 1);
	showSetup->MoveTo (0, hideSetup->Frame().bottom + 1);
	showTopic->MoveTo (0, showSetup->Frame().bottom + 1);
//	altwSetup->MoveTo (0, showTopic->Frame().bottom + 1);
//	altwServer->MoveTo (0, altwSetup->Frame().bottom + 1);

	float biggest (messageBox->Frame().right);

	if (windowFollows->Frame().right > biggest)
		biggest = windowFollows->Frame().right;

	if (hideSetup->Frame().right > biggest)
		biggest = hideSetup->Frame().right;

	if (showSetup->Frame().right > biggest)
		biggest = showSetup->Frame().right;
	
	if (showTopic->Frame().right > biggest)
		biggest = showTopic->Frame().right;
	
//	if (altwSetup->Frame().right > biggest)
//		biggest = altwSetup->Frame().right;
//
//	if (altwServer->Frame().right > biggest)
//		biggest = altwServer->Frame().right;
	
	ResizeTo (biggest, showTopic->Frame().bottom);
}

void
PreferenceWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_MESSAGE_OPEN:
			bowser_app->MessageOpenState (
				messageBox->Value() == B_CONTROL_ON);
			break;

		case M_WINDOW_FOLLOWS:
			bowser_app->WindowFollowsState (
				windowFollows->Value() == B_CONTROL_ON);
			break;
	
		case M_HIDE_SETUP:
			bowser_app->HideSetupState (
				hideSetup->Value() == B_CONTROL_ON);
			break;

		case M_ACTIVATE_SETUP:
			bowser_app->ShowSetupState (
				showSetup->Value() == B_CONTROL_ON);
			break;

		case M_SHOW_TOPIC:
			bowser_app->ShowTopicState (
				showTopic->Value() == B_CONTROL_ON);
			break;
			
		case M_ALTW_SETUP:
			bowser_app->AltwSetupState (
				SetupAltW->Value() == B_CONTROL_ON);
			break;
			
		case M_ALTW_SERVER:
			bowser_app->AltwServerState (
				altwServer->Value() == B_CONTROL_ON);
			break;

		default:
			BView::MessageReceived (msg);
	}
}
