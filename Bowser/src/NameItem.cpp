
#include <Menu.h>
#include <stdio.h>

#include "IRCDefines.h"
#include "Names.h"

NameItem::NameItem (
	const char *name,
	int32 userStatus)

	: BListItem (),
	  myName (name),
	  myAddress (""),
	  myStatus (userStatus)
{
}

NameItem::~NameItem (void)
{
}

BString
NameItem::Name (void) const
{
	return myName;
}

BString
NameItem::Address (void) const
{
	return myAddress;
}

int32
NameItem::Status() const
{
	return myStatus;
}

void
NameItem::SetName (const char *name)
{
	myName = name;
}

void
NameItem::SetAddress (const char *address)
{
	myAddress = address;
}

void
NameItem::SetStatus (int32 userStatus)
{
	myStatus = userStatus;
}

void
NameItem::DrawItem (BView *passedFather, BRect frame, bool complete)
{
	NamesView *father (static_cast<NamesView *>(passedFather));
	
	if (IsSelected())
	{
		father->SetLowColor (180, 180, 180);
		father->FillRect (frame, B_SOLID_LOW);
	}
	else if (complete)
	{
		father->SetLowColor (father->GetColor (C_NAMES_BACKGROUND));
		father->FillRect (frame, B_SOLID_LOW);
	}

	font_height fh;
	father->GetFontHeight (&fh);

	father->MovePenTo (
		frame.left + 4,
		frame.bottom - fh.descent);

	BString drawString (myName);
	rgb_color color = father->GetColor (C_NAMES);

	if ((myStatus & STATUS_OP_BIT) != 0)
	{
		drawString.Prepend ("@");
		color = father->GetColor (C_OP);
	}
	else if ((myStatus & STATUS_VOICE_BIT) != 0)
	{
		drawString.Prepend("+");
		color = father->GetColor (C_VOICE);
	}

	if ((myStatus & STATUS_IGNORE_BIT) != 0)
		color = father->GetColor (C_IGNORE);

	if (IsSelected())
	{
		color.red   = 0;
		color.green = 0;
		color.blue  = 0;
	}

	father->SetHighColor (color);

	father->SetDrawingMode (B_OP_OVER);
	father->DrawString (drawString.String());
	father->SetDrawingMode (B_OP_COPY);
}
