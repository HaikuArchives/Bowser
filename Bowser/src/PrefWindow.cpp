
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
	
	hideServer = new BCheckBox (
		BRect (0, 50, bounds.right, 74),
		"hideserver",
		"Hide setup window on connect",
		new BMessage (M_HIDE_SERVER));
	hideServer->SetValue (bowser_app->GetHideServerState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (hideServer);

	showServer = new BCheckBox (
		BRect (0, 75, bounds.right, 99),
		"showserver",
		"Activate setup window on disconnect",
		new BMessage (M_ACTIVATE_SERVER));
	showServer->SetValue (bowser_app->GetShowServerState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showServer);
	
	showTopic = new BCheckBox (
		BRect (0, 125, bounds.right, 149),
		"showtopic",
		"Show channel topic in Titlebar",
		new BMessage (M_SHOW_TOPIC));
	showTopic->SetValue (bowser_app->GetShowTopicState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showTopic);

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
	hideServer->SetTarget (this);
	showServer->SetTarget (this);
	showTopic->SetTarget (this);

	messageBox->ResizeToPreferred();
	windowFollows->ResizeToPreferred();
	hideServer->ResizeToPreferred();
	showServer->ResizeToPreferred();
	showTopic->ResizeToPreferred();

	windowFollows->MoveTo (0, messageBox->Frame().bottom + 1);
	hideServer->MoveTo (0, windowFollows->Frame().bottom + 1);
	showServer->MoveTo (0, hideServer->Frame().bottom + 1);
	showTopic->MoveTo (0, showServer->Frame().bottom + 1);

	float biggest (messageBox->Frame().right);

	if (windowFollows->Frame().right > biggest)
		biggest = windowFollows->Frame().right;

	if (hideServer->Frame().right > biggest)
		biggest = hideServer->Frame().right;

	if (showServer->Frame().right > biggest)
		biggest = showServer->Frame().right;
	
	if (showTopic->Frame().right > biggest)
		biggest = showTopic->Frame().right;
	
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
	
		case M_HIDE_SERVER:
			bowser_app->HideServerState (
				hideServer->Value() == B_CONTROL_ON);
			break;

		case M_ACTIVATE_SERVER:
			bowser_app->ShowServerState (
				showServer->Value() == B_CONTROL_ON);
			break;

		case M_SHOW_TOPIC:
			bowser_app->ShowTopicState (
				showTopic->Value() == B_CONTROL_ON);
			break;

		default:
			BView::MessageReceived (msg);
	}
}
