
#include <CheckBox.h>

#include "Bowser.h"
#include "PrefLog.h"

PreferenceLog::PreferenceLog (void)
	: BView (
		BRect (0, 0, 270, 150),
	  "PrefLog",
	  B_FOLLOW_LEFT | B_FOLLOW_TOP,
	  B_WILL_DRAW)
{
	BRect bounds (Bounds());

	masterLog = new BCheckBox (
		BRect (0, 0, bounds.right, 24),
		"masterlog",
		"Log to file",
		new BMessage (M_MASTER_LOG));
	masterLog->SetValue (bowser_app->GetMasterLogState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (masterLog);

	dateLogs = new BCheckBox (
		BRect (0, 25, bounds.right, 49),
		"datelogs",
		"Date filenames",
		new BMessage (M_DATE_LOGS));
	dateLogs->SetValue (bowser_app->GetDateLogsState()
		? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild (dateLogs);
}



PreferenceLog::~PreferenceLog (void)
{
}

void
PreferenceLog::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	masterLog->SetTarget (this);
	dateLogs->SetTarget (this);

	masterLog->ResizeToPreferred();
	dateLogs->ResizeToPreferred();

	dateLogs->MoveTo (0, masterLog->Frame().bottom + 1);

	float biggest (masterLog->Frame().right);

	if (dateLogs->Frame().right > biggest)
		biggest = dateLogs->Frame().right;
	
	ResizeTo (biggest, dateLogs->Frame().bottom);
}

void
PreferenceLog::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_MASTER_LOG:
			bowser_app->MasterLogState (
				masterLog->Value() == B_CONTROL_ON);
			break;

		case M_DATE_LOGS:
			bowser_app->DateLogsState (
				dateLogs->Value() == B_CONTROL_ON);
			break;
			
		default:
			BView::MessageReceived (msg);
	}
}
