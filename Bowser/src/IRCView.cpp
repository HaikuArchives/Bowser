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

#define MAX_BYTES           48000 
#define MELTDOWN_BYTES      500000 
#define REMOVE_BYTES        1000

#include <Roster.h>

#include <list>
#include <ctype.h>

#include "VTextControl.h"
#include "Bowser.h"
#include "ClientWindow.h"
#include "IRCView.h"

using namespace std;

struct URL 
{ 
  int32 offset; 
  BString display, 
          url; 
  
  URL (int32 os, const char *buffer) 
    : offset (os), 
      display (buffer), 
      url (buffer) 
      { 
        if (display.ICompare ("https://", 8) == 0) 
        { 
          url  = "http://"; 
          url += display.String() + 8; 
        } 

        if (display.ICompare ("www.", 4) == 0) 
        { 
          url  = "http://"; 
          url += display; 
        } 

        if (display.ICompare ("ftp.", 4) == 0) 
        { 
          url  = "ftp://"; 
          url += display; 
        } 
      } 

  URL (void) 
    : offset (0), 
      display (""), 
      url ("") 
      { 
      } 
}; 


/* We put settings here mostly to eliminate 
 * the template crap through out all 
 * the files that use a IRCView
 */

struct IRCViewSettings 
{ 
  VTextControl  *parentInput; 

  BFont  urlFont; 
  rgb_color  urlColor; 
  list<URL>  urls; 
};


IRCView::IRCView ( 
  BRect textframe, 
  BRect textrect, 
  VTextControl *inputControl) 

  : BTextView ( 
    textframe, 
    "IRCView",
    textrect, 
    B_FOLLOW_ALL_SIDES, 
    B_WILL_DRAW | B_FRAME_EVENTS) 
{ 
  settings = new IRCViewSettings; 

  settings->parentInput = inputControl; 
  settings->urlFont     = *(bowser_app->GetClientFont (F_URL)); 
  settings->urlColor    = bowser_app->GetColor (C_URL); 

  tracking = false;
  MakeEditable (false); 
  MakeSelectable (true); 
  SetStylable (true); 
} 


IRCView::~IRCView (void) 
{ 
  delete settings; 
}


void 
IRCView::MouseDown (BPoint myPoint) 
{ 
  int32 start,
        finish;

  BTextView::MouseDown (myPoint); 
  GetSelection (&start, &finish);
  if (start == finish) 
  { 
    list<URL> &urls (settings->urls); 
    list<URL>::const_iterator it; 
    int32 offset (OffsetAt (myPoint)); 

    for (it = urls.begin(); it != urls.end(); ++it) 

    if (offset >= it->offset 
    &&  offset <  it->offset + it->display.Length()) 
    { 
      const char *arguments[] = {it->url.String(), 0}; 

      be_roster->Launch ("text/html", 
        1, const_cast<char **>(arguments)); 

      settings->parentInput->MakeFocus (true); 
    }
  }
} 


void 
IRCView::KeyDown (const char * bytes, int32 numBytes) 
{ 
  BMessage msg (M_INPUT_FOCUS); 
  BString buffer; 

  buffer.Append (bytes, numBytes); 
  msg.AddString ("text", buffer.String()); 

  Window()->PostMessage(&msg); 
}


int32 
IRCView::DisplayChunk ( 
  const char *cData, 
  const rgb_color *color, 
  const BFont *font) 
{
  int32 urlMarker; 
  BString url, 
          data (cData); 
  bool scrolling (static_cast<ClientWindow *>(Window())->Scrolling()); 

  /* Previously calling SetFontAndColor is really the wrong approach, 
   * since it does not add to the run array.  You have to pass a run array 
   * for this to work.  Otherwise, things like selecting the text 
   * will force the text to redraw in the single run array (you'll lose 
   * all your coloring)
   */

   while ((urlMarker = FirstMarker (data.String())) >= 0) 
   { 
     int32 theLength (URLLength (data.String() + urlMarker)); 
     BString buffer; 

     if (urlMarker) 
     { 
       data.MoveInto (buffer, 0, urlMarker); 

       text_run_array run; 
       run.count          = 1; 
       run.runs[0].offset = 0; 
       run.runs[0].font   = *font; 
       run.runs[0].color  = *color; 

       Insert (TextLength(), buffer.String(), buffer.Length(), &run); 
     } 

     data.MoveInto (buffer, 0, theLength); 

     settings->urls.push_back (URL (TextLength(), buffer.String())); 

     text_run_array run; 

     run.count          = 1; 
     run.runs[0].offset = 0; 
     run.runs[0].font   = settings->urlFont; 
     run.runs[0].color  = settings->urlColor; 

     Insert (TextLength(), buffer.String(), buffer.Length(), &run); 
   } 

   if (data.Length()) 
   { 
     text_run_array run; 
     run.count          = 1; 
     run.runs[0].offset = 0; 
     run.runs[0].font   = *font; 
     run.runs[0].color  = *color; 

     Insert (TextLength(), data.String(), data.Length(), &run); 
   } 

   if (TextLength() > MAX_BYTES) 
   { 
     list<URL> &urls (settings->urls); 
     int32 bytes (REMOVE_BYTES); 
     const char *text (Text()); 

     while (*(text + bytes) && *(text + bytes) != '\n') 
       ++bytes; 

     while (*(text + bytes) 
     &&    (*(text + bytes) == '\n' 
     ||     *(text + bytes) == '\r')) 
       ++bytes; 

     /* woulda, coulda used a deque.. 
      * too many warnings on signed/ 
      * unsigned comparisons -- list 
      * works fine though
      */ 
     while (!urls.empty()) 
       if (urls.front().offset < bytes) 
         urls.erase (urls.begin()); 
       else 
         break; 

     list<URL>::iterator it; 
     for (it = urls.begin(); it != urls.end(); ++it) 
       it->offset -= bytes; 
                
     ClientWindow *parent = (ClientWindow *)Window(); 
     float scrollMin, scrollMax; 
     parent->ScrollRange (&scrollMin, &scrollMax); 

     float scrollVal = parent->ScrollPos(); 
     int32 curLine = (int32) ((scrollMax/LineHeight()) * (scrollVal/scrollMax)); 
                
     Delete (0,bytes); 
     if (!scrolling) 
       parent->SetScrollPos (TextHeight(0, curLine));           
   } 

   if (scrolling && !tracking)
     if (TextLength() > 0) 
       ScrollToOffset (TextLength()); 

   return TextLength(); 
} 

int32
IRCView::URLLength (const char *outTemp) 
{
  BString data (outTemp);
  int32 urlEnd = data.FindFirst(" ");

  if (urlEnd > 0)
    data.Truncate (urlEnd);

  while(data.Length() > 1)
  {
    char last = data.ByteAt (data.Length()-1);
    if (isdigit (last)
    ||  isalpha (last)
    ||  last == '/')
      break;
    else
      data.Truncate (data.Length()-1);
  }

  return data.Length();
} 


int32
IRCView::FirstMarker (const char *cData) 
{ 
  BString data (cData); 
  int32 urlMarkers[6], 
        marker (data.Length()), 
        pos (0); 

  const char *tags[6] = 
  { 
    "http://", 
    "https://", 
    "www.", 
    "ftp://", 
    "ftp.", 
    "file://" 
  }; 

  do 
  { 
    urlMarkers[0] = data.IFindFirst (tags[0], pos); 
    urlMarkers[1] = data.IFindFirst (tags[1], pos); 
    urlMarkers[2] = data.IFindFirst (tags[2], pos); 
    urlMarkers[3] = data.IFindFirst (tags[3], pos); 
    urlMarkers[4] = data.IFindFirst (tags[4], pos); 
    urlMarkers[5] = data.IFindFirst (tags[5], pos); 

    for (int32 i = 0; i < 6; ++i) 
      if (urlMarkers[i] != B_ERROR 
      &&  urlMarkers[i] <  marker) 
      { 
        if (URLLength (cData + urlMarkers[i]) > (int32)strlen (tags[i])) 
        { 
          marker = urlMarkers[i]; 
          pos = data.Length(); 
        } 
        else // just enough 
          pos = urlMarkers[i] + 1; 
      } 
  }
    
  while ((urlMarkers[0] != B_ERROR 
  ||      urlMarkers[1] != B_ERROR 
  ||      urlMarkers[2] != B_ERROR 
  ||      urlMarkers[3] != B_ERROR 
  ||      urlMarkers[4] != B_ERROR 
  ||      urlMarkers[5] != B_ERROR) 
  &&      pos < data.Length()); 

  return marker < data.Length() ? marker : -1; 
} 


void
IRCView::ClearView (bool all) 
{
  if (all || TextLength() < 96)
  {
    // clear all data
    list<URL> &urls (settings->urls); 
    int32 bytes (TextLength());

    while (!urls.empty()) 
      if (urls.front().offset < bytes) 
        urls.erase (urls.begin()); 
      else 
        break; 

    list<URL>::iterator it; 
    for (it = urls.begin(); it != urls.end(); ++it) 
      it->offset -= bytes;

    ClientWindow *parent = (ClientWindow *)Window(); 
    float scrollMin, scrollMax; 
    parent->ScrollRange (&scrollMin, &scrollMax); 

    Delete (0, bytes);
    ScrollToOffset (0);
  }
  else
  {
    // clear all but last couple of lines
    list<URL> &urls (settings->urls); 
    int32 bytes (TextLength() - 96);
    const char *text (Text()); 

    while (*(text + bytes) && *(text + bytes) != '\n') 
      ++bytes;

    while (*(text + bytes) 
    &&    (*(text + bytes) == '\n' 
    ||     *(text + bytes) == '\r')) 
      ++bytes; 

    while (!urls.empty()) 
      if (urls.front().offset < bytes) 
        urls.erase (urls.begin()); 
      else 
        break; 

    list<URL>::iterator it; 
    for (it = urls.begin(); it != urls.end(); ++it) 
      it->offset -= bytes; 

    ClientWindow *parent = (ClientWindow *)Window(); 
    float scrollMin, scrollMax; 
    parent->ScrollRange (&scrollMin, &scrollMax); 

    Delete (0,bytes);
    ScrollToOffset (TextLength());
  }
} 


void 
IRCView::FrameResized (float width, float height) 
{ 
  BRect textrect = TextRect(); 

  textrect.right  = textrect.left + width - 7; 
  textrect.bottom = textrect.top + height - 1; 
  SetTextRect (textrect); 
  ScrollToOffset (TextLength()); 
}


void 
IRCView::SetColor (int32 which, rgb_color color) 
{ 
  if (which == C_URL) 
    settings->urlColor = color; 
}


void 
IRCView::SetFont (int32 which, const BFont *font) 
{ 
  if (which == F_URL) 
    settings->urlFont = *font; 
}
