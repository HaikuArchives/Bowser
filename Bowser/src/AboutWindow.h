#ifndef _ABOUT_WINDOW_H
#define _ABOUT_WINDOW_H

#include <Window.h>

class BTextView;

class AboutWindow : public BWindow
{	
	BTextView				*credits;


	public:

								AboutWindow (const char *);
								~AboutWindow (void);
	virtual void				MessageReceived (BMessage *);
	
	virtual bool			QuitRequested (void);
	virtual void			DispatchMessage (BMessage *, BHandler *);
	void						Pulse (void);
	
	private:
	
	BView *background;
	bool EasterEggOn;
	BView *graphic;
	
};

const uint32 M_ABOUT_CLOSE					= 'abcl';

#endif
