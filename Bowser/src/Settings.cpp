
#include <Window.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <DataIO.h>
#include <File.h>
#include <fs_attr.h>

#include "IRCDefines.h"
#include "Settings.h"

#include <stdio.h>

Settings::Settings (
	const char *label_,
	const char *path_)

	: label (label_),
	  restored (false)
{
	find_directory (B_USER_SETTINGS_DIRECTORY, &path, true);

	path.Append (path_);
}

Settings::~Settings (void)
{
}

void
Settings::Save (void)
{
	BMallocIO mIO;
	ssize_t size;

	packed.MakeEmpty();
	SavePacked (&packed);
	packed.Flatten (&mIO, &size);

	BFile file (path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if (file.InitCheck() == B_NO_ERROR)
	{
		file.WriteAttr (
			label.String(),
			B_MESSAGE_TYPE,
			0,
			mIO.Buffer(),
			mIO.BufferLength());
	}
}

void
Settings::Restore (void)
{
	if (!restored)
	{
		BFile file (path.Path(), B_READ_ONLY);

		if (file.InitCheck() == B_NO_ERROR)
		{
			attr_info info;
			char *buffer;

			if (file.GetAttrInfo (label.String(), &info) == B_NO_ERROR
			&& (buffer = new char [info.size]) != 0)
			{
				if (file.ReadAttr (
					label.String(),
					B_MESSAGE_TYPE,
					0,
					buffer,
					info.size))
				{
					packed.Unflatten (buffer);
				}
	
				delete [] buffer;
			}
		}

		// Do it here, so they can optionally
		// load additional defaults -- they can
		// firgure out it's empty
		RestorePacked (&packed);
		restored = true;
	}
}
	
void
Settings::SavePacked (BMessage *)
{
	printf ("CALLING SETTINGS::SAVEPACKED -- YIKES!\n");
	// Just for the folks down home
}

void
Settings::RestorePacked (BMessage *)
{
	printf ("CALLING SETTINGS::RESTOREPACKED -- YIKES!\n");
	// Just for the folks down home
}


bool
Settings::Restored (void) const
{
	return restored;
}

WindowSettings::WindowSettings (
	BWindow *window_,
	const char *label,
	const char *path)

	: Settings (label, path),
	  window (window_)
{
	notification[0] = notification[1] = notification[2] = true;
}

WindowSettings::~WindowSettings (void)
{
}

BRect
WindowSettings::Frame (void) const
{
	return window->Frame();
}

void
WindowSettings::SavePacked (BMessage *msg)
{
	msg->AddRect ("frame", Frame());

	msg->AddBool ("notification", notification[0]);
	msg->AddBool ("notification", notification[1]);
	msg->AddBool ("notification", notification[2]);
}

void
WindowSettings::RestorePacked (BMessage *msg)
{
	if (msg->HasRect ("frame"))
	{
		BRect frame;

		msg->FindRect ("frame", &frame);
		window->MoveTo (frame.LeftTop());
		window->ResizeTo (frame.Width(), frame.Height());
	}

	for (int32 i = 0; msg->HasBool ("notification", i); ++i)
		msg->FindBool ("notification", i, notification + i);
}
