
#include "Bowser.h"
#include "IRCDefines.h"
#include "ColorLabel.h"
#include "PrefColor.h"

static const char *ControlLabels[] =
{
	"Text:",
	"Background:",
	"Names:",
	"Names Background:",
	"URL:",
	"Server:",
	"Notice:",
	"Action:",
	"Quit:",
	"Error:",
	"Nick:",
	"Me:",
	"Join:",
	"Kick:",
	"Whois:",
	"Op:",
	"Voice:",
	"CTCP Request:",
	"CTCP Reply:",
	"Ignore:",
	0
};

PreferenceColor::PreferenceColor (void)
	: BView (
		BRect (0, 0, 300, 200),
		"ColorPref",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW),

		last_invoke (0),
		had_picker (false)
{
	BRect bounds (Bounds());

	colors = new ColorLabel * [MAX_COLORS];

	float label_width (0.0);

	for (int32 i = 0; ControlLabels[i]; ++i)
		if (StringWidth (ControlLabels[i]) > label_width)
			label_width = StringWidth (ControlLabels[i]);

	font_height fh;
	GetFontHeight (&fh);
	float width (floor ((fh.ascent + fh.descent + fh.leading) * 1.5));

	for (int32 i = 0; i < MAX_COLORS && ControlLabels[i]; ++i)
	{
		BMessage *msg (new BMessage (M_COLOR_INVOKE));

		msg->AddInt32 ("which", i);
		colors[i] = new ColorLabel (
			BRect (
				i % 2 ? floor (bounds.Width() / 2) : 0,
				floor (i / 2) * 30,
				(i % 2 ? floor (bounds.Width() / 2) : 0) + label_width + width + 5,
				floor (i / 2) * 30 + 25),
			"colorlabel",
			ControlLabels[i],
			bowser_app->GetColor (i),
			msg);
		AddChild (colors[i]);
	}
}

PreferenceColor::~PreferenceColor (void)
{
	delete [] colors;
}

void
PreferenceColor::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	for (int32 i = 0; i < MAX_COLORS; ++i)
	{
		colors[i]->ResizeToPreferred();
		colors[i]->SetTarget (this);

		if (i > 1)
		{
			colors[i]->MoveTo (
				colors[i]->Frame().left,
				colors[i - 2]->Frame().bottom + 5);
		}
	}

	ResizeTo (
		colors[1]->Frame().right,
		colors[MAX_COLORS - 1]->Frame().bottom);

	if (had_picker)
	{
		ColorPicker *win;

		win = new ColorPicker (
			pick_point,
			colors[last_invoke]->ValueAsColor(),
			BMessenger (this),
			new BMessage (M_COLOR_CHANGE));
		win->Show();

		picker = BMessenger (win);
	}
}

void
PreferenceColor::DetachedFromWindow (void)
{
	if (picker.IsValid() && picker.LockTarget())
	{
		BLooper *looper;
		
		picker.Target (&looper);
		pick_point = static_cast<BWindow *>(looper)->Frame().LeftTop();
		looper->Unlock();

		picker.SendMessage (B_QUIT_REQUESTED);
		had_picker = true;
	}
}

void
PreferenceColor::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_COLOR_INVOKE:
		{
			msg->FindInt32 ("which", &last_invoke);

			if (!picker.IsValid())
			{
				ColorPicker *win;

				win = new ColorPicker (
					Window()->Frame().RightTop() + BPoint (10, 10),
					colors[last_invoke]->ValueAsColor(),
					BMessenger (this),
					new BMessage (M_COLOR_CHANGE));
				win->Show();

				picker = BMessenger (win);
			}
			else
			{
				BMessage msg (M_COLOR_SELECT);
				rgb_color color;

				color = colors[last_invoke]->ValueAsColor();

				msg.AddData (
					"color",
					B_RGB_COLOR_TYPE,
					&color,
					sizeof (rgb_color));

				picker.SendMessage (&msg);
			}

			break;
		}

		case M_COLOR_CHANGE:
		{
			const rgb_color *color;
			ssize_t size;

			msg->FindData (
				"color",
				B_RGB_COLOR_TYPE,
				reinterpret_cast<const void **>(&color),
				&size);

			colors[last_invoke]->SetColor (*color);
			bowser_app->SetColor (last_invoke, *color);
			break;
		}

		default:
			BView::MessageReceived (msg);
	}
}
