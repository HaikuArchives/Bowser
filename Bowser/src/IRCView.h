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

#ifndef IRCVIEW_H_
#define IRCVIEW_H_

#include <TextView.h>

class BTextControl;
class BFont;

struct IRCViewSettings;

class IRCView : public BTextView
{
  bool     tracking;
  float    lasty;

  public:
    IRCView (
      BRect,
      BRect,
      BTextControl *);

    ~IRCView (void);

    virtual void            MouseDown (BPoint);
    virtual void            KeyDown (
                              const char * bytes,
                              int32 numBytes);
    virtual void            FrameResized (float, float);

    int32                   DisplayChunk (
                              const char *,
                              const rgb_color *,
                              const BFont *);
    int32                   URLLength (const char *);
    int32                   FirstMarker (const char *);

    void                    ClearView (bool);
    void                    SetColor (int32, rgb_color);
    void                    SetFont  (int32, const BFont *);

  private:
    IRCViewSettings         *settings;

};

#endif
