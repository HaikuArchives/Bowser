
#include <Font.h>

#include <stdio.h>

#include "RunView.h"
#include "IRCState.h"

StateInterface::StateInterface (int32 offset_)
	: offset (offset_)
{
}

StateInterface::~StateInterface (void)
{
}

int32
StateInterface::Offset (void) const
{
	return offset;
}

float
StateInterface::GetFontHeight (RunView *) const
{
	return 0;
}

ColorIndexState::ColorIndexState (
	int32 offset_,
	int32 index_,
	ColorGround ground_)

	: StateInterface (offset_),
	  index (index_),
	  ground (ground_)
{
}

ColorIndexState::~ColorIndexState (void)
{
}

void
ColorIndexState::Apply (RunView *runner) const
{
	rgb_color color (runner->GetIndexedColor (index));

	if (ground == ColorForeground)
		runner->SetHighColor (color);
	else
		runner->SetLowColor (color);
}

ColorStaticState::ColorStaticState (
	int32 offset_,
	rgb_color color_,
	ColorGround ground_)

	: StateInterface (offset_),
	  color (color_),
	  ground (ground_)
{
}

ColorStaticState::~ColorStaticState (void)
{
}

void
ColorStaticState::Apply (RunView *runner) const
{
	if (ground == ColorForeground)
		runner->SetHighColor (color);
	else
		runner->SetLowColor (color);
}

FontState::FontState (int32 offset_, int32 index_)
	: StateInterface (offset_),
	  index (index_)
{
}

FontState::~FontState (void)
{
}

void
FontState::Apply (RunView *runner) const
{
	const BFont *font (runner->GetIndexedFont (index));

	runner->SetFont (font);
}

float
FontState::GetFontHeight (RunView *runner) const
{
	const BFont *font (runner->GetIndexedFont (index));
	font_height fh;

	font->GetHeight (&fh);

	return fh.ascent + fh.leading + fh.descent;
}

