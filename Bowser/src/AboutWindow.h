/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * Bowser's source code is provided without any warranty what-so-ever; users may
 * use the source code at their own risk. In no way may the developers who have
 * contributed to the code be held accountable for damages that arise from the use
 * or misuse of the source code.
 * 
 * Anyone is free to use the source code however they like as long as credit is
 * given to The Bowser Team (http://bowser.sourceforge.net) in both the
 * documentation for the program and the source code (if you choose to release it.)
 * Any derivitive work using this code must _not_ use the name Bowser or some sort
 * of variation thereof unless explicit permission is granted from The Bowser Team
 * in advance. In addition, commercial developers may not use this source code
 * without explicit permssion from The Bowser Team in advance.
 *
 */

#ifndef _ABOUT_WINDOW_H
#define _ABOUT_WINDOW_H

#include <Window.h>
#include <Roster.h>

class BTextView;

class AboutWindow : public BWindow
{
  BTextView             *credits;

  public:
                        AboutWindow (void);
                        ~AboutWindow (void);
  
  virtual void          MessageReceived (BMessage *);
  virtual bool          QuitRequested (void);
  virtual void          DispatchMessage (BMessage *, BHandler *);
  void                  Pulse (void);
  void                  AboutImage (const char *, bool);

  private:
    BView               *background;
    bool                EasterEggOn;
    BView               *graphic;
};

class GraphicView : public BView
{
  public:
    GraphicView(BRect frame, const char *name, uint32 resizeMask, uint32 flags) 
      : BView(frame, name, resizeMask, flags)
        {
        };

    virtual void MouseDown (BPoint)
    {
      const char *arguments[] = {"http://bowser.sourceforge.net", 0};
       be_roster->Launch ("text/html", 1, const_cast<char **>(arguments));
     };
};


const uint32 M_ABOUT_CLOSE           = 'abcl'; // The party has to stop sometime
const uint32 M_ABOUT_ORGY            = 'abor'; // Gummy Orgy
const uint32 M_ABOUT_BUDDYJ          = 'abbj'; // Buddy Jesus!

#endif
