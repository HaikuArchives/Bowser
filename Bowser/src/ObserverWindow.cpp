

#include <Application.h>

#include <stdio.h>

#include "PrefWindow.h"
#include "Settings.h"
#include "Bowser.h"
#include "ObserverWindow.h"

ObserverWindow::ObserverWindow (
	BRect frame,
	const char *title,
	window_look look,
	window_feel feel)

	: BWindow (
		frame,
		title,
		look,
		feel,
		B_ASYNCHRONOUS_CONTROLS)
{
	BMessage reply;

	be_app_messenger.SendMessage (M_WINDOW_FOLLOWS, &reply);

	if (reply.FindBool ("windowfollows"))
		SetFlags (Flags()
			| B_NOT_ANCHORED_ON_ACTIVATE
			| B_NO_WORKSPACE_ACTIVATION);

	StartWatching (be_app_messenger, M_WINDOW_FOLLOWS);
}

ObserverWindow::ObserverWindow (
	BRect frame,
	const char *title,
	bool windowfollows,
	window_look look,
	window_feel feel)

	: BWindow (
		frame,
		title,
		look,
		feel,
		B_ASYNCHRONOUS_CONTROLS)
{
	if (windowfollows)
		SetFlags (Flags()
			| B_NOT_ANCHORED_ON_ACTIVATE
			| B_NO_WORKSPACE_ACTIVATION);

	StartWatching (be_app_messenger, M_WINDOW_FOLLOWS);
}


ObserverWindow::~ObserverWindow (void)
{
}

bool
ObserverWindow::QuitRequested (void)
{
	StopWatchingAll (be_app_messenger);

	return BWindow::QuitRequested();
}

void
ObserverWindow::DispatchMessage (BMessage *msg, BHandler *handler)
{
	if (msg->what == B_OBSERVER_NOTICE_CHANGE)
	{
		uint32 what;

		msg->FindInt32 (
			B_OBSERVE_WHAT_CHANGE,
			reinterpret_cast<int32 *>(&what));
		msg->FindInt32 (
			B_OBSERVE_ORIGINAL_WHAT,
			reinterpret_cast<int32 *>(&msg->what));

		if (what == M_STATE_CHANGE)
			StateChange (msg);
		else if (what == M_WINDOW_FOLLOWS)
			SetWindowFollows (msg->FindBool ("windowfollows"));
		else
			NoticeReceived (msg, what);
	}
	else
		BWindow::DispatchMessage (msg, handler);
}

void
ObserverWindow::NoticeReceived (BMessage *, uint32 what)
{
	printf ("Unhandled notice %lu!\n", what);
	//msg->PrintToStream();
}

void
ObserverWindow::StateChange (BMessage *)
{	
	printf ("Unhandled state change!\n");
	//msg->PrintToStream();
}

void
ObserverWindow::SetWindowFollows (bool follows)
{
	if (follows)
		SetFlags (Flags()
			| B_NOT_ANCHORED_ON_ACTIVATE
			| B_NO_WORKSPACE_ACTIVATION);
	else
		SetFlags (Flags()
			& ~(B_NOT_ANCHORED_ON_ACTIVATE
			| B_NO_WORKSPACE_ACTIVATION));
}
