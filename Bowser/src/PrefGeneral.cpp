
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
		"Use same nickname list for all servers",
		new BMessage (M_NICKNAME_BIND));
	nickBindBox->SetValue (bowser_app->GetNicknameBindState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (nickBindBox);

	autoRejoin = new BCheckBox (
		BRect (0, 75, bounds.right, 99),
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
	autoRejoin->SetTarget (this);

	stampBox->ResizeToPreferred();
	paranoidBox->ResizeToPreferred();
	nickBindBox->ResizeToPreferred();
	autoRejoin->ResizeToPreferred();

	paranoidBox->MoveTo (0, stampBox->Frame().bottom + 1);
	nickBindBox->MoveTo (0, paranoidBox->Frame().bottom + 1);
	autoRejoin->MoveTo (0, nickBindBox->Frame().bottom + 1);

	float biggest (stampBox->Frame().right);

	if (paranoidBox->Frame().right > biggest)
		biggest = paranoidBox->Frame().right;

	if (nickBindBox->Frame().right > biggest)
		biggest = nickBindBox->Frame().right;

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

		case M_NICKNAME_BIND:
			bowser_app->NicknameBindState (
				nickBindBox->Value() == B_CONTROL_ON);
			break;

		case M_AUTO_REJOIN:
			bowser_app->AutoRejoinState (
				autoRejoin->Value() == B_CONTROL_ON);
			break;

		default:
			BView::MessageReceived (msg);
	}
}
