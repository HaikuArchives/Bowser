
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <CheckBox.h>
#include <String.h>

#include "VTextControl.h"
#include "Bowser.h"
#include "IRCDefines.h"
#include "PrefNotify.h"

static const char *ControlLabels[] =
{
	"Trigger:",
	"Also known as:",
	"Other nicks:",
	"Auto nick timeout:",
	0
};

PreferenceNotify::PreferenceNotify (void)
	: BView (
		BRect (0, 0, 270, 150),
		"PrefNotify",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
{
	BRect bounds (Bounds());
	BMenu *menu (new BMenu ("Notify"));
	float label_width (0.0);

	for (int32 i = 0; ControlLabels[i]; ++i)
		if (label_width < be_plain_font->StringWidth (ControlLabels[i]))
			label_width = be_plain_font->StringWidth (ControlLabels[i]);

	menu->SetLabelFromMarked (true);
	menu->SetRadioMode (true);

	BMenuItem *item[4];

	menu->AddItem (item[0] = new BMenuItem ("No Deskbar Icon", new BMessage (M_NOTIFY_NONE)));
	menu->AddItem (item[1] = new BMenuItem ("Nickname", new BMessage (M_NOTIFY_NICK)));
	menu->AddItem (item[2] = new BMenuItem ("Content", new BMessage (M_NOTIFY_CONTENT)));
	menu->AddItem (item[3] = new BMenuItem ("Both", new BMessage (M_NOTIFY_COMBO)));

	if ((bowser_app->GetNotificationMask() & NOTIFY_NICK_BIT) != 0
	&&  (bowser_app->GetNotificationMask() & NOTIFY_CONT_BIT) != 0)
		item[3]->SetMarked (true);
	else if ((bowser_app->GetNotificationMask() & NOTIFY_NICK_BIT) != 0)
		item[1]->SetMarked (true);
	else if ((bowser_app->GetNotificationMask() & NOTIFY_CONT_BIT) != 0)
		item[2]->SetMarked (true);
	else
		item[0]->SetMarked (true);

	notifyMenu = new BMenuField (
		BRect (0, 0, bounds.right, 29),
		"notifyMenu",
		ControlLabels[0],
		menu);

	notifyMenu->SetDivider (label_width + 5);
	AddChild (notifyMenu);

	flashNickBox = new BCheckBox (
		BRect (0, 30, bounds.right, 49),
		"flashNick",
		"Flash deskbar icon red for nickname",
		new BMessage (M_NOTIFY_NICK_FLASH));

	flashNickBox->SetValue ((bowser_app->GetNotificationMask() & NOTIFY_NICK_FLASH_BIT)
		? B_CONTROL_ON : B_CONTROL_OFF);
	flashNickBox->SetEnabled ((bowser_app->GetNotificationMask() & NOTIFY_NICK_BIT) != 0);
	AddChild (flashNickBox);
	
	flashContentBox = new BCheckBox (
		BRect (0, 50, bounds.right, 69),
		"flashContent",
		"Flash deskbar icon yellow for content",
		new BMessage (M_NOTIFY_CONT_FLASH));

	flashContentBox->SetValue ((bowser_app->GetNotificationMask() & NOTIFY_CONT_FLASH_BIT)
		? B_CONTROL_ON : B_CONTROL_OFF);
	flashContentBox->SetEnabled ((bowser_app->GetNotificationMask() & NOTIFY_CONT_BIT) != 0);
	AddChild (flashContentBox);

	flashOtherNickBox = new BCheckBox (
		BRect (0, 70, bounds.right, 89),
		"flashOther",
		"Flash deskbar icon blue for other nicks",
		new BMessage (M_NOTIFY_OTHER_FLASH));
	flashOtherNickBox->SetValue ((bowser_app->GetNotificationMask() & NOTIFY_OTHER_FLASH_BIT)
		? B_CONTROL_ON : B_CONTROL_OFF);
	flashOtherNickBox->SetEnabled ((bowser_app->GetNotificationMask() & NOTIFY_NICK_BIT) != 0);
	AddChild (flashOtherNickBox);

	akaEntry = new VTextControl (
		BRect (0, 90, bounds.right, 109),
		"akaEntry",
		ControlLabels[1],
		bowser_app->GetAlsoKnownAs().String(),
		0);
	akaEntry->SetDivider (label_width + 5.0);
	akaEntry->SetModificationMessage (new BMessage (M_AKA_MODIFIED));
	AddChild (akaEntry);

	otherNickEntry = new VTextControl (
		BRect (0, 110, bounds.right, 129),
		"otherNickEntry",
		ControlLabels[2],
		bowser_app->GetOtherNick().String(),
		0);
	otherNickEntry->SetDivider (label_width + 5.0);
	otherNickEntry->SetModificationMessage (new BMessage (M_OTHER_NICK_MODIFIED));
	AddChild (otherNickEntry);

	autoNickTimeEntry = new VTextControl (
		BRect (0, 130, bounds.right, 149),
		"autoNickTimeEntry",
		ControlLabels[3],
		bowser_app->GetAutoNickTime().String(),
		0);
	autoNickTimeEntry->SetDivider (label_width + 5.0);
	autoNickTimeEntry->SetModificationMessage (new BMessage (M_AUTO_NICK_MODIFIED));
	AddChild (autoNickTimeEntry);
}

PreferenceNotify::~PreferenceNotify (void)
{
}

void
PreferenceNotify::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	notifyMenu->Menu()->SetTargetForItems (this);
	flashNickBox->SetTarget (this);
	flashContentBox->SetTarget (this);
	flashOtherNickBox->SetTarget (this);
	akaEntry->SetTarget (this);
	otherNickEntry->SetTarget (this);
	autoNickTimeEntry->SetTarget (this);

	flashNickBox->ResizeToPreferred();
	flashContentBox->ResizeToPreferred();
	flashOtherNickBox->ResizeToPreferred();

	flashNickBox->MoveTo (0, notifyMenu->Frame().bottom + 10);
	flashContentBox->MoveTo (0, flashNickBox->Frame().bottom + 1);
	flashOtherNickBox->MoveTo (0, flashContentBox->Frame().bottom + 1);
	akaEntry->MoveTo (0, flashOtherNickBox->Frame().bottom + 10);
	otherNickEntry->MoveTo (0, akaEntry->Frame().bottom + 1);
	autoNickTimeEntry->MoveTo (0, otherNickEntry->Frame().bottom + 1);

	ResizeTo (Frame().Width(), autoNickTimeEntry->Frame().bottom);
}

void
PreferenceNotify::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NOTIFY_NONE:
			bowser_app->NotificationMask (
				bowser_app->GetNotificationMask()
				& ~(NOTIFY_NICK_BIT | NOTIFY_CONT_BIT));
					
			flashNickBox->SetEnabled (false);
			flashContentBox->SetEnabled (false);
			flashOtherNickBox->SetEnabled (false);
			break;

		case M_NOTIFY_NICK:
			bowser_app->NotificationMask (
				(bowser_app->GetNotificationMask()
				| NOTIFY_NICK_BIT) & ~(NOTIFY_CONT_BIT));

			flashNickBox->SetEnabled (true);
			flashOtherNickBox->SetEnabled (true);
			flashContentBox->SetEnabled (false);
			break;

		case M_NOTIFY_CONTENT:
			bowser_app->NotificationMask (
				(bowser_app->GetNotificationMask()
				| NOTIFY_CONT_BIT) & ~(NOTIFY_NICK_BIT));

			flashNickBox->SetEnabled (false);
			flashOtherNickBox->SetEnabled (false);
			flashContentBox->SetEnabled (true);
			break;

		case M_NOTIFY_COMBO:
			bowser_app->NotificationMask (
				(bowser_app->GetNotificationMask()
				| NOTIFY_NICK_BIT | NOTIFY_CONT_BIT));

			flashNickBox->SetEnabled (true);
			flashOtherNickBox->SetEnabled (true);
			flashContentBox->SetEnabled (true);
			break;

		case M_NOTIFY_NICK_FLASH:
			if (flashNickBox->Value() == B_CONTROL_ON)
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					| NOTIFY_NICK_FLASH_BIT));
			else
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					& ~(NOTIFY_NICK_FLASH_BIT)));
			break;

		case M_NOTIFY_CONT_FLASH:
			if (flashContentBox->Value() == B_CONTROL_ON)
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					| NOTIFY_CONT_FLASH_BIT));
			else
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					& ~(NOTIFY_CONT_FLASH_BIT)));
			break;

		case M_NOTIFY_OTHER_FLASH:
			if (flashOtherNickBox->Value() == B_CONTROL_ON)
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					| NOTIFY_OTHER_FLASH_BIT));
			else
				bowser_app->NotificationMask (
					(bowser_app->GetNotificationMask()
					& ~(NOTIFY_OTHER_FLASH_BIT)));
			break;

		case M_AKA_MODIFIED:
			bowser_app->AlsoKnownAs (akaEntry->TextView()->Text());
			break;

		case M_OTHER_NICK_MODIFIED:
			bowser_app->OtherNick (otherNickEntry->TextView()->Text());
			break;

		default:
			BView::MessageReceived (msg);
	}
}
