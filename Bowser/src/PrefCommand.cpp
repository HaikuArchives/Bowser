
#include <TextControl.h>
#include <String.h>

#include <stdio.h>

#include "Bowser.h"
#include "PrefCommand.h"

static const char *ControlLabels[] =
{
	"Quit:",
	"Kick:",
	"Ignore:",
	"Unignore:",
	"Away:",
	"Back:",
	"Uptime:",
	0
};


PreferenceCommand::PreferenceCommand (void)
	: BView (
		BRect (0, 0, 270, 150),
		"PrefFont",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
{
	BRect bounds (Bounds());
	float label_width (0.0);

	for (int32 i = 0; ControlLabels[i]; ++i)
		if (StringWidth (ControlLabels[i]) > label_width)
			label_width = StringWidth (ControlLabels[i]);

	commands = new BTextControl * [MAX_COMMANDS];

	for (int32 i = 0; i < MAX_COMMANDS; ++i)
	{
		commands[i] = new BTextControl (
			BRect (0, i * 25, bounds.right, i * 25 + 20),
			"commands",
			ControlLabels[i],
			bowser_app->GetCommand (i).String(),
			0);

		commands[i]->SetDivider (label_width + 5);
		
		BMessage *msg (new BMessage (M_COMMAND_MODIFIED));

		msg->AddInt32 ("which", i);
		commands[i]->SetModificationMessage (msg);
		AddChild (commands[i]);
	}
}

PreferenceCommand::~PreferenceCommand (void)
{
	delete [] commands;
}

void
PreferenceCommand::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	for (int32 i = 0; i < MAX_COMMANDS; ++i)
	{
		if (i)
			commands[i]->MoveTo (0, commands[i - 1]->Frame().bottom + 1);
		commands[i]->SetTarget (this);
	}

	ResizeTo (Bounds().Width(), commands[MAX_COMMANDS - 1]->Frame().bottom);
}

void
PreferenceCommand::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_COMMAND_MODIFIED:
		{
			int32 which;

			msg->FindInt32 ("which", &which);
			bowser_app->SetCommand (
				which,
				commands[which]->TextView()->Text());
			break;
		}

		default:
			BView::MessageReceived (msg);
	}
}
