
#include <Application.h>
#include <Window.h>
#include <ScrollView.h>

#include "RunView.h"

class MyApp : public BApplication
{
	int32				repeated;

	public:

						MyApp (void);
	virtual			~MyApp (void);
	virtual void	ArgvReceived (int32, char **);
	virtual void	ReadyToRun (void);
	virtual bool	QuitRequested (void);
};

class MyWindow : public BWindow
{
	public:

						MyWindow (int32);
	virtual			~MyWindow (void);
	virtual bool	QuitRequested (void);
};


MyApp::MyApp (void)
	: BApplication ("application/x-vnd.TVLP-RunView"),
	  repeated (1)
{
}

MyApp::~MyApp (void)
{
}

void
MyApp::ArgvReceived (int32 ac, char **av)
{
	if (ac > 1)
		repeated = atoi (av[1]);
}

void
MyApp::ReadyToRun (void)
{
	(new MyWindow (repeated))->Show();
}

bool
MyApp::QuitRequested (void)
{
	return true;
}

MyWindow::MyWindow (int32 repeated)
	: BWindow (
		BRect (100, 100, 500, 500),
		"Run View",
		B_DOCUMENT_WINDOW,
		0)
{
	BRect frame (Bounds());

	frame.InsetBy (1.0, 1.0);
	frame.right -= B_V_SCROLL_BAR_WIDTH - 1.0;
	RunView *runview (new RunView (frame));

	for (int32 i = 0; i < repeated; ++i)
	{
	runview->Append (
		"For \00303applications where \00309,03"
		"performance isn't\00305,00 critical\00301, other APIs "
		"allow you to calculate the minimal bounding \00307box "
		"of a glyph or a string, either on the screen "
		"(the rectangle is rounded to an integer and "
		"takes all distortions into account) ");
	runview->Append (
		"or as \00308p\00313ri\00311,10nted (original ", 0, 2);
	runview->Append (
		"floating-point values)\00311,00.  You can "
		"switch between the two "
		"modes \00300,12by using the font_metric_mode\00301,00 ", 0, 1);
	runview->Append (
		"parameter (B_SCREEN_METRIC or "
		"B_PRINTING_METRIC).\n\n");

	runview->Append ("You betcha\n\n");

	runview->Append ("The function ", 0, 3);
	runview->Append ("exists in \00301different \00302flavors:  ");
	runview->Append ("\00303bounding \00304boxes \00305for ");
	runview->Append ("\00306individual \00307glyphs ");
	runview->Append ("\00308(GetBoundingBoxesAsGlyphs) \00309or ");
	runview->Append ("\00310whole \00311words \00312(GetBoundingBoxesAsString ");
	runview->Append ("\00313for ", 0, 3);
	runview->Append ("\00314a \00315single \00300,01string, \00300,02or ");
	runview->Append ("\00300,03GetBoundingBoxesForStrings \00300,04to ");
	runview->Append ("\00300,05process \00300,06many \00300,07strings ");
	runview->Append ("\00300,08in ");
	runview->Append ("\00300,09one \00300,10call).  \00300,11To ");
	runview->Append ("\00300,12convert \00300,13those \00300,14rectangles ");
	runview->Append ("\00300,15to screen coordinates, you need to ");
	runview->Append ("apply the regular offset formula (no -1.0 needed).\n\n");


  	runview->Append ("This memo defines an Experimental Protocol for the Internet ");
  	runview->Append ("community.  Discussion and suggestions for improvement are requested. ");
  	runview->Append ("Please refer to the current edition of the \"IAB Official Protocol ");
  	runview->Append ("Standards\" for the standardization state and status of this protocol. ");
  	runview->Append ("Distribution of this memo is unlimited.\n\n");

	runview->Append ("Initializes the BPopUpMenu object.  If the object is added to a BMenuBar, its name ");
	runview->Append ("also becomes the initial label of its controlling item (just as for other BMenus).  ");
	runview->Append ("If the labelFromMarked flag is true (as it is by default), the label of the controlling ");
	runview->Append ("item will change to reflect the label of the item that the user last selected.  In addition, ");
	runview->Append ("the menu will operate in radio mode (regardless of the value passed as the ");
	runview->Append ("radioMode flag).  When the menu pops up, it will position itself so that the marked ");
	runview->Append ("item appears directly over the controlling item in the BMenuBar.\n");
	runview->Append ("If labelFromMarked is false, the menu pops up so that its first item is over the controling item.\n");
	runview->Append ("If the radioMode flag is true (as it is by default), the last item selected by the user ");
	runview->Append ("will always be marked.  In this mode, one and only one item within the menu can be ");
	runview->Append ("marked at a time.  If radioMode is false, items aren't automatically marked or unmarked.\n");

	}

	BScrollView *scroller (new BScrollView (
		"scroller",
		runview,
		B_FOLLOW_ALL_SIDES,
		0,
		false,
		true,
		B_PLAIN_BORDER));
	AddChild (scroller);

	runview->MakeFocus (true);
}

MyWindow::~MyWindow (void)
{
}

bool
MyWindow::QuitRequested (void)
{
	be_app->PostMessage (B_QUIT_REQUESTED);
	return true;
}

int
main (void)
{
	new MyApp;

	be_app->Run();
	delete be_app;

	return 0;
}
