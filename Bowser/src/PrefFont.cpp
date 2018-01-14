
#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Font.h>

#include <stdio.h>
#include <string.h>

#include "Bowser.h"
#include "PrefFont.h"

struct FontStat
{
	font_family				family;
	int32						style_count;
	font_style				*styles;
};

static const char *ControlLabels[] =
{
	"Text:",
	"Server:",
	"URL:",
	"Names:",
	"Input:",
	0
};

static const int32 FontSizes[] =
{
	6, 8, 9, 10, 11, 12, 14, 15, 16, 18, 24, 36, 48, 72, 0
};

PreferenceFont::PreferenceFont (void)
	: BView (
		BRect (0, 0, 270, 150),
		"PrefFont",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
{
	clientFont[0] = 0;
}

PreferenceFont::~PreferenceFont (void)
{
}

void
PreferenceFont::AttachedToWindow (void)
{
	BView::AttachedToWindow();

	SetViewColor (Parent()->ViewColor());

	
	// Moved this here because Gord has 3000 fonts,
	// and he was complaining that the preferences
	// window was coming up slow.  This will postpone it
	// until he wants to change the fonts.  Can you believe it,
	// 3000!?!?
	if (clientFont[0] == 0)
	{
		int32 i, family_count (count_font_families());
		float label_width (0.0);

		for (i = 0; ControlLabels[i]; ++i)
			if (be_plain_font->StringWidth (ControlLabels[i]) > label_width)
				label_width = be_plain_font->StringWidth (ControlLabels[i]);

	
		FontStat *font_stat = new FontStat [family_count];

		for (i = 0; i < family_count; ++i)
		{
			uint32 flags;

			*font_stat[i].family       = '\0';
			font_stat[i].style_count   = 0;
			font_stat[i].styles        = 0;

			if (get_font_family (i, &font_stat[i].family, &flags) == B_OK
			&& (font_stat[i].style_count = count_font_styles (font_stat[i].family)) > 0)
			{
				font_stat[i].styles = new font_style [font_stat[i].style_count];
			
				for (int32 j = 0; j < font_stat[i].style_count; ++j)
				{
					*font_stat[i].styles[j] = '\0';
					get_font_style (font_stat[i].family, j, font_stat[i].styles + j, &flags);
				}
			}
		}

		BMenu *parentMenu[2][5];
		BRect frame (Bounds());
		font_height fh;
		float height;

		GetFontHeight (&fh);
		height = fh.ascent + fh.descent + fh.leading + 20;

		for (i = 0; i < 5; ++i)
		{
			font_family cur_family;
			font_style  cur_style;
			float cur_size;
			int32 j;

			bowser_app->GetClientFont (i)->GetFamilyAndStyle (
				&cur_family,
				&cur_style);
			cur_size = bowser_app->GetClientFont (i)->Size();

			parentMenu[0][i] = new BMenu ("Font");
			parentMenu[1][i] = new BMenu ("Size");

			clientFont[i] = new BMenuField (
				BRect (0, i * height, 185, 20 + i * height),
				"clientFont",
				ControlLabels[i],
				parentMenu[0][i]);

			AddChild (clientFont[i]);

			fontSize[i] = new BMenuField (
				BRect (210, i * height, frame.right, 20 + i * height),
				"fontSize",
				"",
				parentMenu[1][i]);
			fontSize[i]->Menu()->SetRadioMode (true);
			fontSize[i]->SetDivider (0.0);
	
			AddChild (fontSize[i]);

			clientFont[i]->SetDivider (label_width + 5.0);

			for (j = 0; j < family_count; ++j)
				if (*font_stat[j].family && font_stat[j].style_count)
				{
					BMenu *menu (new BMenu (font_stat[j].family));
					parentMenu[0][i]->AddItem (menu);

					for (int32 k = 0; k < font_stat[j].style_count; ++k)
					{
						BMessage *msg (new BMessage (M_FONT_CHANGE));
						BMenuItem *item;

						msg->AddString ("family", font_stat[j].family);
						msg->AddString ("style", font_stat[j].styles[k]);
						msg->AddInt32 ("which", i);

						menu->AddItem (item = new BMenuItem (font_stat[j].styles[k], msg));
	
						if (strcmp (font_stat[j].family,     cur_family) == 0
						&&  strcmp (font_stat[j].styles[k],  cur_style)  == 0)
						{
							item->SetMarked (true);
							clientFont[i]->MenuItem()->SetLabel (font_stat[j].family);
						}
					}
				}

			for (j = 0; FontSizes[j]; ++j)
			{
				BMessage *msg (new BMessage (M_FONT_SIZE_CHANGE));
				BMenuItem *item;
				char buffer[32];
	
				sprintf (buffer, "%ld", FontSizes[j]);
				msg->AddInt32 ("size", FontSizes[j]);
				msg->AddInt32 ("which", i);
	
				parentMenu[1][i]->AddItem (item = new BMenuItem (buffer, msg));
	
				if (FontSizes[j] == cur_size)
					item->SetMarked (true);
			}
	
		}

		for (i = 0; i < family_count; ++i)
			delete [] font_stat[i].styles;
		delete [] font_stat;
	}

	for (int32 i = 0; i < 5; ++i)
	{
		fontSize[i]->Menu()->SetLabelFromMarked (true);
		fontSize[i]->MenuItem()->SetLabel (fontSize[i]->Menu()->FindMarked()->Label());

		fontSize[i]->Menu()->SetTargetForItems (this);

		for (int32 j = 0; j < clientFont[i]->Menu()->CountItems(); ++j)
		{
			BMenuItem *item (clientFont[i]->Menu()->ItemAt (j));

			item->Submenu()->SetTargetForItems (this);
		}
	}

	ResizeTo (Frame().Width(), clientFont[4]->Frame().bottom);
}

void
PreferenceFont::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_FONT_CHANGE:
		{
			const char *family, *style;
			BMenuItem *item;
			int32 which;

			msg->FindInt32 ("which", &which);

			// Unmark
			for (int32 i = 0; i < clientFont[which]->Menu()->CountItems(); ++i)
			{
				BMenu *menu (clientFont[which]->Menu()->SubmenuAt (i));

				if ((item = menu->FindMarked()) != 0)
				{
					item->SetMarked (false);
					break;
				}
			}

			msg->FindPointer ("source", reinterpret_cast<void **>(&item));
			item->SetMarked (true);

			msg->FindString ("family", &family);
			msg->FindString ("style", &style);

			clientFont[which]->MenuItem()->SetLabel (family);
			bowser_app->ClientFontFamilyAndStyle (which, family, style);
			break;
		}

		case M_FONT_SIZE_CHANGE:
		{
			int32 which, size;

			msg->FindInt32 ("which", &which);
			msg->FindInt32 ("size", &size);
			bowser_app->ClientFontSize (which, size);
		}

		default:
			BView::MessageReceived (msg);
	}
}
