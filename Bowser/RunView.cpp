
#include <Application.h>
#include <String.h>
#include <Region.h>

#include <vector>

#include <stdio.h>
#include <ctype.h>


#include <ScrollView.h>
#include <ScrollBar.h>
#include <Region.h>
#include <StopWatch.h>

#include "IRCState.h"
#include "RunView.h"

// FontBreaks indicate where in the RunBuffer a font
// change has taken place.  At this point, we are only
// interested in the height of the font.  This information
// is used to determine how much space (in height) a given line
// needs when drawing the RunLine

class FontBreak
{
	public:

	int32								start;
	int32								length;
	int32								index;
	font_height						fh;

	
										FontBreak (
											int32 s,
											int32 l,
											int32 i,
											const font_height &f)
											: start (s),
											  length (l),
											  index (i)
										{
											fh.ascent  = f.ascent;
											fh.descent = f.descent;
											fh.leading = f.leading;
										}

	bool 								operator == (const font_height &f)
										{
											return fh.ascent  == f.ascent
											&&     fh.descent == f.descent
											&&		 fh.leading == f.leading;
										}

	float								Height (void) const
										{
											return ceil (fh.ascent + fh.descent + fh.leading);
										}
};

// An instance of a RunBreak is an indication of where
// the RunView can break up text to format the line,
// so that it fits in the bounds of the runview.
// (Usually where whitespace starts in the string buffer)
// This information is cached in an attempt to speed up
// resizing the window.

class RunBreak
{
	public:

	int32								offset;		// measurement in pixels
														// at this position if
														// runbuffer was on one line
	float								position;	// index of string buffer
														// where this whitespace is at
};

// A RunLine is a line of text in the RunView.  It is a class which
// defines a portion of the RunBuffer (one line).  There will be
// an instance of a RunLine for every soft and hard line terminator
// in a RunBuffer.

class RunLine
{
	public:

	int32								start;
	int32								length;

	BRect								frame;
};


// A RunBuffer is a buffer of text terminated by the '\n' character
// (a hard line terminator).  
class RunBuffer
{
	public:

	BString 							line;

	vector<RunBreak *>			runBreaks;
	vector<FontBreak *>			fontBreaks;
	vector<StateInterface *>	states;
	vector<RunLine *>				lines;

	BRect								frame;
	float								totalWidth;

	RunBuffer						*next,
										*prev;

										RunBuffer (
											RunView *,
											RunBuffer *,
											const char *,
											int32,
											int32,
											int32);

										~RunBuffer (void);

	void								Append (
											RunView *,
											const char *,
											int32,
											int32,
											int32);

	int32								BiggestFont (int32, int32);
	void								CalcFrame (float);

	rgb_color						mIRCColor (int32) const;
};

RunView::RunView (BRect frame)
	: BView (
		frame,
		"RunView",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),

		firstBuffer (0),
		lastBuffer  (0),
		spacer (0.0),
		scroller (0),
		tracking (false)
{
	for (int32 i = 0; i < MAX_FONTS; ++i)
	{
		fonts[i] = *be_plain_font;
		fonts[i].SetSize (12 + i * 2);
	}

	SetFont (fonts);
	
	rgb_color black = {0, 0, 0, 255};
	for (int32 i = 0; i < MAX_COLORS; ++i)
		colors[i] = black;
}

RunView::~RunView (void)
{
	while (firstBuffer)
	{
		RunBuffer *next (firstBuffer->next);

		delete firstBuffer;
		firstBuffer = next;
	}
}

void
RunView::FrameResized (float width, float height)
{
	RunBuffer *buffer;

	for (buffer = firstBuffer; buffer; buffer = buffer->next)
		buffer->CalcFrame (width);
	UpdateScrollBar();

	// Recalc Run array
	Draw (Bounds());
}

void
RunView::TargetedByScrollView (BScrollView *scrollView)
{
	scroller = scrollView;
}

void
RunView::AttachedToWindow (void)
{
	UpdateScrollBar();
}

void
RunView::UpdateScrollBar (void)
{
	if (scroller)
	{
		BScrollBar *bar (scroller->ScrollBar (B_VERTICAL));

		if (bar)
		{
			bar->SetSteps (20.0, Bounds().Height());

			if (lastBuffer && !lastBuffer->lines.empty())
			{
				bar->SetRange (0.0, max_c (0.0, lastBuffer->lines.back()->frame.bottom - Bounds().Height()));
				bar->SetProportion (min_c (1.0, Bounds().Height() / lastBuffer->lines.back()->frame.bottom));
			}
			else
				bar->SetRange (0.0, 0.0);
		}
	}
}

BRect
RunView::SelectionFrame (void)
{
	int32 line[2] = {-1, -1};
	BRect frame;

	if (selection[0].runner && selection[1].runner)
	{
		for (int i = 0; i < 2; ++i)
		{
			for (uint32 j = 0; j < selection[i].runner->lines.size(); ++j)
			{
				if (selection[i].offset >= selection[i].runner->lines[j]->start
				&&  selection[i].offset <  selection[i].runner->lines[j]->start
					+ selection[i].runner->lines[j]->length)
				{
					line[i] = j;
					break;
				}
			}
		}
	}

	if (line[0] >= 0 && line[1] >= 0)
	{
		frame.top    = selection[0].runner->lines[line[0]]->frame.top;
		frame.bottom = selection[1].runner->lines[line[1]]->frame.bottom;

		float x[2];

		for (int i = 0; i < 2; ++i)
		{
			x[i] = selection[i].runner->lines[line[i]]->frame.left;
			BFont *font (fonts + 0);
			uint32 fb (0);
		
			for (int32 j = selection[i].runner->lines[line[i]]->start;
				j < selection[i].offset;
				++j)
			{
				while (fb < selection[i].runner->fontBreaks.size() && selection[i].runner->fontBreaks[fb]->start <= j)
					font = fonts + selection[i].runner->fontBreaks[fb++]->index;

				x[i] += font->StringWidth (selection[i].runner->line.String() + j, 1);
			}
		}

		frame.left  = min_c (x[0], x[1]);
		frame.right = max_c (x[0], x[1]);
	}

	return frame;
}

void
RunView::DeselectAll (void)
{
	if (selection[0].runner && selection[1].runner)
	{
		BRect frame (SelectionFrame());

		selection[0].runner = selection[1].runner = 0;

		Draw (frame);
	}
}

// Yep.. the drawing is not efficient.  I know.  However
// there is virtually no flicker, and resizing is very
// fluid.  This was the goal and this is what my knoggin
// allowed me to come up with
void
RunView::Draw (BRect frame)
{
	const rgb_color viewColor = (ViewColor());
	BRect bounds (Bounds());
	RunBuffer *buffer, *lastDrawn (0);
	bool select_start (false);

	SetHighColor (0, 0, 0, 255);
	SetLowColor (ViewColor());

	for (buffer = firstBuffer; buffer; buffer = buffer->next)
	{
		if (IsFocus()
		&&  Window()->IsActive()
		&&  buffer == selection[0].runner
		&&  selection[1].runner)	
			select_start = true;

		if (buffer->frame.Intersects (frame))
		{
			uint32 state (0);

			PushState();

			for (uint32 i = 0; i < buffer->lines.size(); ++i)
			{
				if (buffer->lines[i]->frame.Intersects (frame))
				{
					float x (buffer->lines[i]->frame.left);
					int32 place (buffer->lines[i]->start);
					int32 bigFont (buffer->BiggestFont (buffer->lines[i]->start, buffer->lines[i]->length));

					PushState();
					SetHighColor (ViewColor());
					FillRect (
						BRect (
							0.0,
							buffer->lines[i]->frame.top,
							buffer->lines[i]->frame.left - 1.0,
							buffer->lines[i]->frame.bottom));
					PopState();

					while (place < buffer->lines[i]->start + buffer->lines[i]->length)
					{
						int32 length (buffer->lines[i]->length - (place - buffer->lines[i]->start));
							
						while (state < buffer->states.size()
						&&     buffer->states[state]->Offset() <= place)
							buffer->states[state++]->Apply (this);

						if (state < buffer->states.size()
						&&  buffer->states[state]->Offset() < buffer->lines[i]->start + buffer->lines[i]->length)
							length = buffer->states[state]->Offset() - place;

						if (select_start)
						{
							if (selection[0].runner == buffer)
							{
								// First runner that is selected!  We have to consider
								// the portion that precedes the selection, the selection
								// and the text after selection (if all contained on one line)
								if (place < selection[0].offset)
								{
									int32 notSelLen (min_c (length, selection[0].offset - place));
									float sWidth (StringWidth (buffer->line.String() + place, notSelLen));

									DrawPortion (buffer, i, x, sWidth, place, notSelLen, bigFont);

									x      += sWidth;
									place  += notSelLen;
									length -= notSelLen;
								}

								int32 selLen (0);

								if (selection[1].runner == buffer)
									selLen = min_c (length, selection[1].offset - place + 1);
								else
									selLen = min_c (length, buffer->lines[i]->start + buffer->lines[i]->length - place);


								if (length && selLen > 0)
								{
									float sWidth (StringWidth (buffer->line.String() + place, selLen));

									DrawSelection (buffer, i, x, sWidth, place, selLen, bigFont);

									x      += sWidth;
									place  += selLen;
									length -= selLen;
								}
							}
							else if (selection[1].runner == buffer)
							{
								if (place <= selection[1].offset)
								{
									// has to be a two or more line
									// selection, so assume from the get go
									// that we need to draw selection part
									int32 selLen (min_c (
											length,
											selection[1].offset - place + 1));

									float sWidth (StringWidth (buffer->line.String() + place, selLen));

									DrawSelection (buffer, i, x, sWidth, place, selLen, bigFont);

									x      += sWidth;
									place  += selLen;
									length -= selLen;
								}
							}
							else
							{
								// The whole line is selected
								float sWidth (StringWidth (buffer->line.String() + place, length));

								DrawSelection (buffer, i, x, sWidth, place, length, bigFont);

								x      += sWidth;
								place  += length;
								length =  0;
							}
						}

						if (length)
						{
							float sWidth (StringWidth (buffer->line.String() + place, length));

							DrawPortion (buffer, i, x, sWidth, place, length, bigFont);
								
							x     += sWidth;
							place += length;
						}
					}

					lastDrawn = buffer;

					PushState();
					SetHighColor (ViewColor());
					FillRect (
						BRect (
							x,
							buffer->lines[i]->frame.top,
							bounds.right,
							buffer->lines[i]->frame.bottom));
					PopState();
				}
			}

			PopState();
		}

		if (buffer == selection[1].runner)
			select_start = false;
	}

	SetDrawingMode (B_OP_COPY);

	if (lastDrawn && lastDrawn->frame.bottom < frame.bottom)
	{
		SetHighColor (ViewColor());

		FillRect (
			BRect (
				frame.left,
				lastDrawn->frame.bottom + 1,
				frame.right,
				frame.bottom));
	}
}

void
RunView::DrawPortion (
	RunBuffer *buffer,
	int32 line,
	float x,
	float width,
	int32 place,
	int32 length,
	int32 bigFont)
{
	SetDrawingMode (B_OP_COPY);
	FillRect (
		BRect (
			x,
			buffer->lines[line]->frame.top,
			x + width,
			buffer->lines[line]->frame.bottom),
		B_SOLID_LOW);
								
	SetDrawingMode (B_OP_OVER);
	DrawString (
		buffer->line.String() + place,
		length,
		BPoint (
			x,
			buffer->lines[line]->frame.bottom
				- buffer->fontBreaks[bigFont]->fh.descent));
}

void
RunView::DrawSelection (
	RunBuffer *buffer,
	int32 line,
	float x,
	float width,
	int32 place,
	int32 length,
	int32 bigFont)
{
	PushState();

	rgb_color hi  = (HighColor());
	rgb_color low = (LowColor());

	hi.red    = 255 - hi.red;
	hi.green  = 255 - hi.green;
	hi.blue   = 255 - hi.blue;

	low.red   = 255 - low.red;
	low.green = 255 - low.green;
	low.blue  = 255 - low.blue;

	SetHighColor (hi);
	SetLowColor (low);

	DrawPortion (
		buffer,
		line,
		x,
		width,
		place,
		length,
		bigFont);

	PopState();
}

void
RunView::MouseDown (BPoint point)
{
	BMessage *msg (Window()->CurrentMessage());
	uint32 buttons, modifiers;

	if (!IsFocus()) MakeFocus (true);

	msg->FindInt32 ("buttons", (int32 *)&buttons);
	msg->FindInt32 ("modifiers", (int32 *)&modifiers);

	if (buttons != B_PRIMARY_MOUSE_BUTTON
	|| (modifiers & B_OPTION_KEY)  != 0
	|| (modifiers & B_COMMAND_KEY) != 0
	|| (modifiers & B_CONTROL_KEY) != 0)
	{
		DeselectAll();
		return;
	}

	bool gotShift ((modifiers & B_SHIFT_KEY) != 0);
	bool seenFirst (false), seenLast (false);

	RunBuffer *runner;
	uint32 line (0);

	for (runner = firstBuffer; runner; runner = runner->next)
	{
		if (runner == selection[0].runner)
			seenFirst = true;

		if (runner == selection[1].runner)
			seenLast = true;

		if (runner->frame.Contains (point))
		{
			for (line = 0; line < runner->lines.size(); ++line)
				if (runner->lines[line]->frame.Contains (point))
					break;
			break;
		}
	}

	if (runner
	&&  line < runner->lines.size()
	&&  point.x >= runner->lines[line]->frame.left)
	{
		float x (runner->lines[line]->frame.left);
		BFont *font (fonts + 0);
		uint32 fb (0);
		
		for (int32 i = runner->lines[line]->start;
			i < runner->lines[line]->length + runner->lines[line]->start;
			++i)
		{
			while (fb < runner->fontBreaks.size() && runner->fontBreaks[fb]->start <= i)
				font = fonts + runner->fontBreaks[fb++]->index;

			x += font->StringWidth (runner->line.String() + i, 1);
			if (x > point.x)
			{
				BRegion rSelect;

				int32 clicks;
				msg->FindInt32 ("clicks", &clicks);

				if (selection[0].runner && selection[1].runner)
					rSelect.Include (SelectionFrame());

				if (clicks % 3 == 0 && clicks)
				{
					if  (!gotShift
					||  (gotShift
					&&   seenFirst
					&& ((selection[0].runner == runner && i > selection[0].offset)
					||   selection[0].runner != runner)))
					{
						if (!gotShift)
						{
							// line selection
							selection[0].runner = runner;
							selection[0].offset = runner->lines[line]->start;
						}

						selection[1].runner = runner;
						selection[1].offset = runner->lines[line]->start + runner->lines[line]->length - 1;
					}
					else
					{
						selection[0].runner = runner;
						selection[0].offset = runner->lines[line]->start;
					}

					rSelect.Include (SelectionFrame());
				}
				else if (clicks % 2 == 0 && clicks)
				{
					if  (!gotShift
					||  (gotShift
					&&   seenFirst
					&& ((selection[0].runner == runner && i > selection[0].offset)
					||   selection[0].runner != runner)))
					{
						for (int j = 0; j < 2; ++j)
						{
							int mod (j ? 1 : (-1));

							if ((!gotShift && j == 0) || j)
							{
								// word selection
								selection[j].runner = runner;
								selection[j].offset = i;

								if (isalpha (runner->line[i]) || isdigit (runner->line[i]))
									while (selection[j].offset + mod >= 0
									&&     selection[j].offset + mod < runner->line.Length()
									&&    (isalpha (runner->line[selection[j].offset + mod])
									||     isdigit (runner->line[selection[j].offset + mod])))
										selection[j].offset += mod;
							}
						}
					}
					else
					{
						// Extending upward by a word
						selection[0].runner = runner;
						selection[0].offset = i;

						if (isalpha (runner->line[i]) || isdigit (runner->line[i]))
							while (selection[0].offset - 1 >= 0
							&&    (isalpha (runner->line[selection[0].offset - 1])
							||     isdigit (runner->line[selection[0].offset - 1])))
								selection[0].offset--;
					}

					rSelect.Include (SelectionFrame());
				}
				else // Single clicks
				{
					if (gotShift)
					{
						if  (seenFirst
						&& ((selection[0].runner == runner && i >= selection[0].offset)
						||   selection[0].runner != runner))
						{
							selection[1].runner = runner;
							selection[1].offset = i;

							if (!selection[0].runner)
							{
								// Hmmm, what to do?
								selection[0].runner = runner;
								selection[0].offset = i;
							}
						}
						else
						{
							// we are extending the selection upwards
							selection[0].runner = runner;
							selection[0].offset = i;

							if (!selection[1].runner)
							{
								selection[1].runner = runner;
								selection[1].offset = i;
							}
						}

						rSelect.Include (SelectionFrame());
					}
					else
					{
						DeselectAll();
						trackSelection.runner = runner;
						trackSelection.offset = i;
						tracking = true;

						// We can't keep up, no sense in trying
						SetMouseEventMask (B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
						return;
					}
				}

				for (int32 k = 0; k < rSelect.CountRects(); ++k)
					Draw (rSelect.RectAt (k));

				break;
			}
		}
	}
}

void
RunView::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	if (tracking && transit == B_INSIDE_VIEW)
	{
		bool seenTrack (false);
		RunBuffer *runner;
		uint32 line (0);

		for (runner = firstBuffer; runner; runner = runner->next)
		{
			if (runner == trackSelection.runner)
				seenTrack = true;

			if (point.y >= runner->frame.top
			&&  point.y <= runner->frame.bottom)
			{
				for (line = 0; line < runner->lines.size(); ++line)
					if (point.y >= runner->lines[line]->frame.top
					&&  point.y <= runner->lines[line]->frame.bottom)
						break;
				break;
			}
		}

		if (runner && line < runner->lines.size())
		{
			//&&  point.x >= runner->lines[line]->frame.left)
			float x (runner->lines[line]->frame.left);
			BFont *font (fonts + 0);
			uint32 fb (0);
			int32 i (0);

			if (point.x < runner->lines[line]->frame.left)
			{
				// We are to the left of a line, so
				// the selection ends at the end
				// of the previous line
				if (line)
					--line;
				else if ((runner = runner->prev) != 0)
					line = runner->lines.size() - 1;

				if (runner)
				{
					i = runner->lines[line]->start + runner->lines[line]->length - 1;
					x = runner->lines[line]->frame.right;
				}
			}
			else
			{
				// We are either in a line or to the right of it
				for (i = runner->lines[line]->start;
					i < runner->lines[line]->length + runner->lines[line]->start;
					++i)
				{
					while (fb < runner->fontBreaks.size() && runner->fontBreaks[fb]->start <= i)
						font = fonts + runner->fontBreaks[fb++]->index;

					x += font->StringWidth (runner->line.String() + i, 1);
					if (x > point.x)
						break;
				}
			}
				
			BRegion rSelect;

			if (runner && seenTrack)
			{
				BRegion eRegion;

				if (selection[0].runner && selection[1].runner)
					eRegion.Include (SelectionFrame());
							
				// Going down
				selection[0].runner = trackSelection.runner;
				selection[0].offset = trackSelection.offset;
				selection[1].runner = runner;
				selection[1].offset = min_c (
					i,
					runner->lines[line]->length
					+ runner->lines[line]->start - 1);

				rSelect.Include (SelectionFrame());

				if (eRegion.CountRects())
				{
					// At this point, we want to only
					// draw pixels outside the intersection
					// of both eRegion and rSelect
					BRegion common (eRegion);

					// This is what they have in common
					common.IntersectWith (&rSelect);
					rSelect.Include (&eRegion);
					rSelect.Exclude (&common);
				}

				printf ("************\nOld selection ");
				eRegion.PrintToStream();

				printf ("New Selection ");
				SelectionFrame().PrintToStream();
				printf ("Redrawing ");
				rSelect.PrintToStream();
			}
			else if (runner)
			{
				// Going up
				selection[0].runner = runner;
				selection[0].offset = i;
				selection[1].runner = trackSelection.runner;
				selection[1].offset = trackSelection.offset;
			}

			for (int32 k = 0; k < rSelect.CountRects(); ++k)
				Draw (rSelect.RectAt (k));
		}
	}
}

void
RunView::MouseUp (BPoint point)
{
	tracking = false;
}

void
RunView::KeyDown (const char *bytes, int32 numBytes)
{
	printf ("key down %.*s\n", (int) numBytes, bytes);
}

void
RunView::MakeFocus (bool focused)
{
	BView::MakeFocus (focused);

	if (selection[0].runner && selection[1].runner)
		Draw (SelectionFrame());
}

void
RunView::WindowActivated (bool active)
{
	BView::WindowActivated (active);

	if (selection[0].runner && selection[1].runner)
		Draw (SelectionFrame());
}

void
RunView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case B_COPY:
			
			if (selection[0].runner
			&&  selection[1].runner)
			{
				BString buffer;

				if (selection[0].runner == selection[1].runner)
				{
					buffer.Append (
						selection[0].runner->line.String() + selection[0].offset,
						selection[1].offset - selection[0].offset + 1);
				}
				else
				{
					RunBuffer *runner (selection[0].runner);

					buffer.Append (runner->line.String() + selection[0].offset);

					do
					{
						if ((runner = runner->next) != 0)
						{
							if (runner == selection[1].runner)
							{
								buffer.Append (runner->line.String(), selection[1].offset + 1);
							}
							else
							{
								buffer.Append (runner->line.String());
							}
						}

					} while (runner && runner != selection[1].runner);
				}

				if (be_clipboard->Lock())
				{
					be_clipboard->Clear();
					BMessage *clipping (be_clipboard->Data());

					clipping->AddData (
						"text/plain",
						B_MIME_TYPE,
						buffer.String(),
						buffer.Length() + 1);

					be_clipboard->Commit();
					be_clipboard->Unlock();
				}
			}
			break;

		case B_SELECT_ALL:

			if (firstBuffer
			&& (selection[0].runner != firstBuffer
			||  selection[0].offset != 0
			||  selection[1].runner != lastBuffer
			||  selection[1].offset != lastBuffer->lines.back()->start
				+ lastBuffer->lines.back()->length - 1))
			{
				selection[0].runner = firstBuffer;
				selection[0].offset = 0;

				selection[1].runner = lastBuffer;
				selection[1].offset = lastBuffer->lines.back()->start
					+ lastBuffer->lines.back()->length - 1;

				Draw (Bounds());
			}
			break;

		default:
			BView::MessageReceived (msg);
	}
}

void
RunView::Append (
	const char *chunk,
	int32 color_index,
	int32 font_index,
	bool  timestamp)
{
	const char *place;

	do
	{
		place = strchr (chunk, '\n');
		int32 length (place ? (place - chunk) + 1 : strlen (chunk));

		if (lastBuffer
		&&  lastBuffer->line.Length()
		&&  *(lastBuffer->line.String()
				+ lastBuffer->line.Length() - 1) != '\n')
		{
			lastBuffer->Append (
				this,
				chunk,
				length,
				color_index,
				font_index);
		}
		else
		{
			spacer = 0.0;
			lastBuffer = new RunBuffer (
				this,
				lastBuffer,
				chunk,
				length,
				color_index,
				font_index);

			if (!firstBuffer)
				firstBuffer = lastBuffer;
		}

		chunk = place + 1;

		
		if (place)
			// Here we have a hard break, so we calculate the frame
			// of the buffer
			lastBuffer->CalcFrame (Bounds().Width());

	} while (place && *(place + 1));
}

rgb_color
RunView::GetIndexedColor (int32 which) const
{
	if (which < 0 || which >= MAX_COLORS)
		which = 0;

	return colors[which];
}

const BFont *
RunView::GetIndexedFont (int32 which) const
{
	if (which < 0 || which >= MAX_FONTS)
		which = 0;

	return fonts + which;
}

RunBuffer::RunBuffer (
	RunView *runner,
	RunBuffer *lastBuffer,
	const char *chunk,
	int32 length,
	int32 color_index,
	int32 font_index)

	: frame (10, 0, runner->Bounds().right - 10, 0),
	  totalWidth (0.0),
	  next (0),
	  prev (0)
{
	if (lastBuffer)
	{
		lastBuffer->next = this;
		prev = lastBuffer;

		frame.top = lastBuffer->frame.bottom + 1;
	}

	Append (
		runner,
		chunk,
		length,
		color_index,
		font_index);
}

RunBuffer::~RunBuffer (void)
{
	while (!runBreaks.empty())
	{
		RunBreak *rb (runBreaks.back());

		runBreaks.erase (runBreaks.begin()
			+ runBreaks.size() - 1);
		delete rb;
	}

	while (!fontBreaks.empty())
	{
		FontBreak *fb (fontBreaks.back());

		fontBreaks.erase (fontBreaks.begin()
			+ fontBreaks.size() - 1);
		delete fb;
	}

	while (!states.empty())
	{
		StateInterface *state (states.back());

		states.erase (states.begin()
			+ states.size() - 1);
		delete state;
	}

	while (!lines.empty())
	{
		RunLine *rl (lines.back());

		lines.erase (lines.begin()
			+ lines.size() - 1);
		delete rl;
	}
}

void
RunBuffer::Append (
	RunView *runner,
	const char *chunk,
	int32 length,
	int32 color_index,
	int32 font_index)
{
	const BFont *font (runner->GetIndexedFont (font_index));
	const char *place;
	int32 existing (line.Length()), last (line.Length());


	//BRect rBounds (runner->Bounds());

	states.push_back (new ColorIndexState (existing, color_index));
	states.push_back (new FontState (existing, font_index));

	font_height rfh;
	font->GetHeight (&rfh);

	if (fontBreaks.empty()
	|| !((*fontBreaks.back()) == rfh))
		fontBreaks.push_back (new FontBreak (
			existing,
			length,
			font_index,
			rfh));

	// Find out where we can break the chunk up
	for (place = chunk; *place && place - chunk < length; ++place)
	{
		if (isspace (*place))
		{
			RunBreak *rb (new RunBreak);

			rb->offset   = line.Length();
			rb->position = runner->spacer
				+ font->StringWidth (line.String() + last);

			line += *place;
			runBreaks.push_back (rb);

			while (place + 1 - chunk < length
			&&   *(place + 1)
			&&     isspace (*(place + 1)))
			{
				++place;
				line += *place;
			}

			runner->spacer += font->StringWidth (line.String() + last);
			last = line.Length();
		}

		// mIRC codes (Move to translation class)
		else if (*place == '\003')
		{
			int32 index;

			++place;
			index = *place++ - '0';

			if (isdigit (*place))
				index = index * 10 + *place++ - '0';

			states.push_back (new ColorStaticState (
				line.Length(),
				mIRCColor (index)));

			if (*place == ',' && isdigit (*(place + 1)))
			{
				++place;
				index = *place++ - '0';

				if (isdigit (*place))
					index = index * 10 + *place++ - '0';

				states.push_back (new ColorStaticState (
					line.Length(),
					mIRCColor (index),
					ColorBackground));
			}

			// we're one too many
			--place;
		}
		else
			line += *place;
	}

	totalWidth += font->StringWidth (line.String() + existing);
}

void
RunBuffer::CalcFrame (float viewWidth)
{
	float x (10.0);
	float mod (0.0);
	int32 last (0);

	while (!lines.empty())
	{
		delete lines.back();
		lines.erase (lines.begin() + (lines.size() - 1));
	}

	frame.left   = 10.0;
	frame.right  = viewWidth - 10.0;
	frame.top    = prev ? prev->frame.bottom + 1.0 : 0.0;

	for (uint32 i = 0; i < runBreaks.size(); ++i)
	{
		if (runBreaks[i]->position - mod >= viewWidth - 10 - x)
		{
			float y (lines.empty() ? frame.top : lines.back()->frame.bottom + 1);

			while (last && isspace (*(line.String() + last)))
				++last;

			lines.push_back (new RunLine);

			lines.back()->start        = last;
			lines.back()->length       = runBreaks[i - 1]->offset - last;
			lines.back()->frame.left   = x;
			lines.back()->frame.top    = y;
			lines.back()->frame.right  = x + runBreaks[i - 1]->position - mod;
			lines.back()->frame.bottom = y + fontBreaks[BiggestFont (lines.back()->start, lines.back()->length)]->Height();
			mod                        = runBreaks[i - 1]->position;
			last								= runBreaks[i - 1]->offset;
			x                          = 30;
		}
	}

	while (last && isspace (*(line.String() + last)))
		++last;

	if (last < line.Length())
	{
		float y (lines.empty() ? frame.top : lines.back()->frame.bottom + 1);

		lines.push_back (new RunLine);

		lines.back()->start           = last;
		lines.back()->length				= line.Length() - last - 1;
		lines.back()->frame.left		= x;
		lines.back()->frame.top       = y;
		lines.back()->frame.right     = x + runBreaks.back()->position - mod;
		lines.back()->frame.bottom    = y + fontBreaks[BiggestFont (lines.back()->start, lines.back()->length)]->Height();
	}

	frame.bottom = lines.back()->frame.bottom;
}

rgb_color
RunBuffer::mIRCColor (int32 index) const
{
	rgb_color color = {255, 255, 255, 255};

	switch (index % 16)
	{
		case 1:
			// black
			color.red = color.green = color.blue = 0;
			break;
			
		case 2:
			// dark blue
			color.red = color.green = 39;
			color.blue = 104;
			break;

		case 3:
			// dark green
			color.red = color.blue = 39;
			color.green = 104;
			break;

		case 4:
			// Bright red
			color.green = color.blue = 39;
			color.red = 234;
			break;
			
		case 5:
			// Brown
			color.red = color.green = color.blue = 104;
			break;

		case 6:
			// Dark purple
			color.red = color.blue = 124;
			color.green = 10;
			break;

		case 7:
			// Orange
			color.red = 220;
			color.green = 150;
			color.blue = 115;
			break;

		case 8:
			// Yellow
			color.red = color.green = 235;
			color.blue = 155;
			break;

		case 9:
			// Bright Green
			color.red = color.blue = 155;
			color.green = 235;
			break;

		case 10:
			// Dark Cyan
			color.green = color.blue = 144;
			color.red = 40;
			break;

		case 11:
			// Light Cyan
			color.green = color.blue = 205;
			color.red = 40;
			break;
			
		case 12:
			// Light Blue
			color.red = 40;
			color.green = 118;
			color.blue = 205;
			break;
			
		case 13:
			// Pink
			color.red = 235;
			color.green = 115;
			color.blue = 205;
			break;

		case 14:
			// Dark Gray
			color.red = color.green = color.blue = 90;
			break;

		case 15:
			// Light Gray
			color.red = color.green = color.blue = 170;
			break;
	}

	return color;
}

int32
RunBuffer::BiggestFont (int32 start, int32 length)
{
	int32 big (0);

	for (uint32 i = 1; i < fontBreaks.size(); ++i)
	{
		if (fontBreaks[i]->start > start + length)
			break;

		if (fontBreaks[i]->start <= start
		||  fontBreaks[big]->Height() < fontBreaks[i]->Height())
			big = i;
	}

	return big;
}

RunSelection::RunSelection (void)
	: runner (0),
	  offset (0)
{
}

RunSelection::~RunSelection (void)
{
}
