#ifndef _ABOUT_WINDOW_H
#define _ABOUT_WINDOW_H

#include <Window.h>
#include <Roster.h>

class BTextView;

class AboutWindow : public BWindow
{	
	BTextView				*credits;


	public:

								AboutWindow (void);
								~AboutWindow (void);
	virtual void				MessageReceived (BMessage *);
	
	virtual bool			QuitRequested (void);
	virtual void			DispatchMessage (BMessage *, BHandler *);
	void						Pulse (void);
	void				ResetImage (void);
	void				EggImage (const char *);

	private:
	
	BView *background;
	bool EasterEggOn;
	BView *graphic;
	
};

class GraphicView : public BView
{
	public:
	GraphicView(BRect frame, const char *name, uint32 resizeMask, uint32 flags) 
		: BView(frame, name, resizeMask, flags)
	{
	};
	
	virtual void MouseDown(BPoint)
	{
		const char *arguments[] = {"http://bowser.sourceforge.net", 0};
		be_roster->Launch ("text/html", 1, const_cast<char **>(arguments));
	};
	
};


const uint32 M_ABOUT_CLOSE					= 'abcl'; // The party has to stop sometime
const uint32 M_ABOUT_ORGY					= 'abor'; // Gummy Orgy
const uint32 M_ABOUT_BUDDYJ					= 'abbj'; // Buddy Jesus!

#endif
