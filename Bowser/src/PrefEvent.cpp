
#include <TextControl.h>

#include <stdio.h>

#include "Bowser.h"
#include "PrefEvent.h"

static const char *ControlLabels[] =
{
	"Join:",
	"Part:",
	"Nick:",
	"Quit:",
	"Kick:",
	"Topic:",
	"Server Notice:",
	"User Notice:",
	"Notify On:",
	"Notify Off:",
	0
};


PreferenceEvent::PreferenceEvent (void)
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

	events = new BTextControl * [MAX_EVENTS];

	for (int32 i = 0; i < MAX_EVENTS; ++i)
	{
		events[i] = new BTextControl (
			BRect (0, i * 25, bounds.right, i * 25 + 20),
			"event",
			ControlLabels[i],
			bowser_app->GetEvent (i).String(),
			0);

		events[i]->SetDivider (label_width + 5);
		
		BMessage *msg (new BMessage (M_EVENT_MODIFIED));

		msg->AddInt32 ("which", i);
		events[i]->SetModificationMessage (msg);
		AddChild (events[i]);
	}
}

PreferenceEvent::~PreferenceEvent (void)
{
	delete [] events;
}

void
PreferenceEvent::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	for (int32 i = 0; i < MAX_EVENTS; ++i)
	{
		if (i)
			events[i]->MoveTo (0, events[i - 1]->Frame().bottom + 1);
		events[i]->SetTarget (this);
	}

	ResizeTo (Bounds().Width(), events[MAX_EVENTS - 1]->Frame().bottom);
}

void
PreferenceEvent::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_EVENT_MODIFIED:
		{
			int32 which;

			msg->FindInt32 ("which", &which);
			bowser_app->SetEvent (
				which,
				events[which]->TextView()->Text());
			break;
		}

		default:
			BView::MessageReceived (msg);
	}
}
