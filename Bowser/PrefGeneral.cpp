
#include <CheckBox.h>

#include "Bowser.h"
#include "PrefGeneral.h"

PreferenceGeneral::PreferenceGeneral (void)
	: BView (
		BRect (0, 0, 270, 150),
	  "PrefGeneral",
	  B_FOLLOW_LEFT | B_FOLLOW_TOP,
	  B_WILL_DRAW)
{
	BRect bounds (Bounds());

	stampBox = new BCheckBox (
		BRect (0, 0, bounds.right, 24),
		"stamp",
		"Timestamp messages",
		new BMessage (M_STAMP_BOX));

	stampBox->SetValue (bowser_app->GetStampState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (stampBox);

	paranoidBox = new BCheckBox (
		BRect (0, 25, bounds.right, 49),
		"paranoid",
		"Omit processor info from version reply",
		new BMessage (M_STAMP_PARANOID));
	paranoidBox->SetValue (bowser_app->GetParanoidState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (paranoidBox);

	nickBindBox = new BCheckBox (
		BRect (0, 50, bounds.right, 74),
		"nickbind",
		"Bind nicknames to server",
		new BMessage (M_NICKNAME_BIND));
	nickBindBox->SetValue (bowser_app->GetNicknameBindState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (nickBindBox);

	messageBox = new BCheckBox (
		BRect (0, 75, bounds.right, 99),
		"message",
		"Message command opens new query window",
		new BMessage (M_MESSAGE_OPEN));
	messageBox->SetValue (bowser_app->GetMessageOpenState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (messageBox);

	windowFollows = new BCheckBox (
		BRect (0, 125, bounds.right, 149),
		"windowfollow",
		"Window selection follows workspace",
		new BMessage (M_WINDOW_FOLLOWS));
	windowFollows->SetValue (bowser_app->GetWindowFollowsState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (windowFollows);
	
	hideServer = new BCheckBox (
		BRect (0, 150, bounds.right, 174),
		"hideserver",
		"Hide setup window on connect",
		new BMessage (M_HIDE_SERVER));
	hideServer->SetValue (bowser_app->GetHideServerState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (hideServer);

	showServer = new BCheckBox (
		BRect (0, 175, bounds.right, 199),
		"showserver",
		"Activate setup window on disconnect",
		new BMessage (M_ACTIVATE_SERVER));
	showServer->SetValue (bowser_app->GetShowServerState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showServer);
	
	showTopic = new BCheckBox (
		BRect (0, 200, bounds.right, 224),
		"showtopic",
		"Show channel topic in Titlebar",
		new BMessage (M_SHOW_TOPIC));
	showTopic->SetValue (bowser_app->GetShowTopicState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (showTopic);

	autoRejoin = new BCheckBox (
		BRect (0, 225, bounds.right, 249),
		"autorejoin",
		"Automagically re-join channels when kicked",
		new BMessage (M_AUTO_REJOIN));
	autoRejoin->SetValue (bowser_app->GetAutoRejoinState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (autoRejoin);
}



PreferenceGeneral::~PreferenceGeneral (void)
{
}

void
PreferenceGeneral::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	stampBox->SetTarget (this);
	paranoidBox->SetTarget (this);
	nickBindBox->SetTarget (this);
	messageBox->SetTarget (this);
	windowFollows->SetTarget (this);
	hideServer->SetTarget (this);
	showServer->SetTarget (this);
	showTopic->SetTarget (this);
	autoRejoin->SetTarget (this);

	stampBox->ResizeToPreferred();
	paranoidBox->ResizeToPreferred();
	nickBindBox->ResizeToPreferred();
	messageBox->ResizeToPreferred();
	windowFollows->ResizeToPreferred();
	hideServer->ResizeToPreferred();
	showServer->ResizeToPreferred();
	showTopic->ResizeToPreferred();
	autoRejoin->ResizeToPreferred();

	paranoidBox->MoveTo (0, stampBox->Frame().bottom + 1);
	nickBindBox->MoveTo (0, paranoidBox->Frame().bottom + 1);
	messageBox->MoveTo (0, nickBindBox->Frame().bottom + 1);
	windowFollows->MoveTo (0, messageBox->Frame().bottom + 1);
	hideServer->MoveTo (0, windowFollows->Frame().bottom + 1);
	showServer->MoveTo (0, hideServer->Frame().bottom + 1);
	showTopic->MoveTo (0, showServer->Frame().bottom + 1);
	autoRejoin->MoveTo (0, showTopic->Frame().bottom + 1);

	float biggest (stampBox->Frame().right);

	if (paranoidBox->Frame().right > biggest)
		biggest = paranoidBox->Frame().right;

	if (nickBindBox->Frame().right > biggest)
		biggest = nickBindBox->Frame().right;

	if (messageBox->Frame().right > biggest)
		biggest = messageBox->Frame().right;

	if (windowFollows->Frame().right > biggest)
		biggest = windowFollows->Frame().right;

	if (hideServer->Frame().right > biggest)
		biggest = hideServer->Frame().right;

	if (showServer->Frame().right > biggest)
		biggest = showServer->Frame().right;
	
	if (showTopic->Frame().right > biggest)
		biggest = showTopic->Frame().right;
	
	if (autoRejoin->Frame().right > biggest)
		biggest = autoRejoin->Frame().right;

	ResizeTo (biggest, autoRejoin->Frame().bottom);
}

void
PreferenceGeneral::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_STAMP_BOX:
			bowser_app->StampState (
				stampBox->Value() == B_CONTROL_ON);
			break;

		case M_STAMP_PARANOID:
			bowser_app->ParanoidState (
				paranoidBox->Value() == B_CONTROL_ON);
			break;

		case M_MESSAGE_OPEN:
			bowser_app->MessageOpenState (
				messageBox->Value() == B_CONTROL_ON);
			break;

		case M_NICKNAME_BIND:
			bowser_app->NicknameBindState (
				nickBindBox->Value() == B_CONTROL_ON);
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

		case M_AUTO_REJOIN:
			bowser_app->AutoRejoinState (
				autoRejoin->Value() == B_CONTROL_ON);
			break;

		default:
			BView::MessageReceived (msg);
	}
}
