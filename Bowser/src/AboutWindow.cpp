
#include <Screen.h>
#include <View.h>
#include <TextView.h>
#include <Bitmap.h>
#include <Font.h>
#include <TranslationUtils.h>
#include <Message.h>
#include <Application.h>

#include <stdio.h>

#include "AboutWindow.h"
#include "IRCDefines.h"

AboutWindow::AboutWindow (const char *version)
	: BWindow (
		BRect (0.0, 0.0, 317.0, 225.0),
		"About Bowser",
		B_TITLED_WINDOW,
		B_WILL_DRAW | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect bounds (Bounds());
	BBitmap *bmp;
	
	// what's a program without easter eggs?
	AddShortcut('O', B_COMMAND_KEY, new BMessage(M_ABOUT_ORGY));
	AddShortcut('J', B_COMMAND_KEY, new BMessage(M_ABOUT_BUDDYJ));

	background = new BView (
		bounds,
		"background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW);
	background->SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));
	AddChild (background);
	

	if ((bmp = BTranslationUtils::GetBitmap ('bits', "bits")) != 0)
	{
		BRect bmp_bounds (bmp->Bounds());

		ResizeTo (
			bmp_bounds.Width() + 50,
			bmp_bounds.Height() + 250);

		graphic = new BView (
			bmp->Bounds().OffsetByCopy (25, 25),
			"image",
			B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW);
		background->AddChild (graphic);
		
		graphic->SetViewBitmap (bmp);
		EasterEggOn = true;
		delete bmp;

		bounds.Set (
			0.0,
			graphic->Frame().bottom + 1, 
			Bounds().right,
			Bounds().bottom);
	};
			
	credits = new BTextView (
		bounds,
		"credits",
		bounds.OffsetToCopy (B_ORIGIN).InsetByCopy (20, 0),
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW); 
	
	credits->SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));
	credits->MakeSelectable (false);
	credits->MakeEditable (false);
	credits->SetStylable (true);
	credits->SetAlignment (B_ALIGN_CENTER);
	background->AddChild (credits);
	
	const char *creditstext =
		"\n\n\n\n\n\n\n\n\n"
		"History\n\n"
		"Once upon a time, there were lots of dinosaurs and stuff.  Er, fast-forward "
		"to 1999.  There were two IRC clients for BeOS: Baxter and Felix.  They both "
		"had their merits, but Andrew Bazan didn't like either.  So he embarked on "
		"writing his own.  He entitled it Bowser.  Eventually, he had too much other "
		"crap going for him, so he open sourced it.  Now, there are a myriad of developers "
		"working to constantly improve upon it.\n\n"
		"Developers\n\n"
		"Andrew Bazan (Hiisi),\nRene Gollent (AnEvilYak)\nTodd Lair (tlair),\nBrian Luft (Electroly),\nWade Majors (kurros),\n and Jamie Wilkinson (project).\n\n"
		"Thanks\n\n"
		"Special thanks go out to: "
		"Gord McLeod, Flax, BeTika, Resartus, "
		"mneptok, AnEvilYak, and Be, Inc.\n\n"
		"http://bowser.sourceforge.net\n\n\n\n\n\n\n\n"
		"So, like, two guys walk into a bar.\n\n\n\n\nAnd they fell down.\n\n\n\n\nHAW!"
		"\n\n\n";

	rgb_color black = {0, 0, 0, 255};
	BFont font (*be_plain_font);
	text_run_array run;

	font.SetSize (font.Size() * 2.0);
	run.count          = 1;
	run.runs[0].offset = 0;
	run.runs[0].font   = font;
	run.runs[0].color  = black;

	credits->Insert ("\n\n\n\n\n\n\n\n\n", &run);
	credits->Insert (version, &run);
	credits->Insert ("\n", &run);

	run.runs[0].font = *be_plain_font;
	credits->Insert (creditstext, &run);
	
	
	// Center that bad boy
	BRect frame (BScreen().Frame());
	MoveTo (
		frame.Width()/2  -  Frame().Width()/2,
		frame.Height()/2 - Frame().Height()/2); 

	SetPulseRate (80000);
}

AboutWindow::~AboutWindow(void)
{
}

bool
AboutWindow::QuitRequested (void)
{
	be_app_messenger.SendMessage (M_ABOUT_CLOSE);
	return true;
}

void
AboutWindow::DispatchMessage (BMessage *msg, BHandler *handler)
{
	if (msg->what == B_PULSE)
		Pulse();

	BWindow::DispatchMessage (msg, handler);
}

void
AboutWindow::Pulse (void)
{
	BPoint point (credits->PointAt (credits->TextLength() - 1));
	credits->ScrollBy (0, 1);

	if (credits->Bounds().bottom > point.y + Bounds().Height())
		credits->ScrollTo (0, 0);
}

void
AboutWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		
		case M_ABOUT_ORGY:
		{
			BBitmap *bmp;	
			if (EasterEggOn) {
				
				if ((bmp = BTranslationUtils::GetBitmap ('bits', "bits")) != 0)
				{
					BRect bmp_bounds (bmp->Bounds());
					
					graphic->ResizeTo (
						bmp_bounds.Width(),
						bmp_bounds.Height());
						
					graphic->SetViewBitmap (bmp);
					
					credits->MoveTo (
						0.0,
						graphic->Frame().bottom + 1);
					
					graphic->Invalidate();
					EasterEggOn = false;
					delete bmp;
	
				}
			}
			else
			{		  		
		  		if ((bmp = BTranslationUtils::GetBitmap ('bits', "Gummy Orgy")) != 0)
				{
					BRect bmp_bounds (bmp->Bounds());
					
					graphic->ResizeTo (
						bmp_bounds.Width(),
						bmp_bounds.Height());
						
					graphic->SetViewBitmap (bmp);
					
					credits->MoveTo (
						0.0,
						graphic->Frame().bottom + 1);
					
					graphic->Invalidate();
					EasterEggOn = true;
					delete bmp;
	
				}
			}
			break;
		}
		
		case M_ABOUT_BUDDYJ:
		{
			BBitmap *bmp;			
			if (EasterEggOn) {
				
				if ((bmp = BTranslationUtils::GetBitmap ('bits', "bits")) != 0)
				{
					BRect bmp_bounds (bmp->Bounds());
					
					graphic->ResizeTo (
						bmp_bounds.Width(),
						bmp_bounds.Height());
						
					graphic->SetViewBitmap (bmp);
					
					credits->MoveTo (
						0.0,
						graphic->Frame().bottom + 1);
					
					graphic->Invalidate();
					EasterEggOn = false;
					delete bmp;
	
				}
			}
			else
			{		  		
		  		if ((bmp = BTranslationUtils::GetBitmap ('bits', "Buddy Jesus")) != 0)
				{
					BRect bmp_bounds (bmp->Bounds());
					
					graphic->ResizeTo (
						bmp_bounds.Width(),
						bmp_bounds.Height());
						
					graphic->SetViewBitmap (bmp);
					
					credits->MoveTo (
						0.0,
						graphic->Frame().bottom + 1);
					
					graphic->Invalidate();
					EasterEggOn = true;
					delete bmp;
	
				}
			}
			break;
		}
	
	};		
		
}

