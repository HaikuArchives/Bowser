#define MAX_BYTES			32000 
#define MELTDOWN_BYTES		500000 
#define REMOVE_BYTES		1000

#include <Alert.h>
#include <TextControl.h>
#include <Font.h>
#include <Roster.h>

#include <list>
#include <stdio.h>
#include <ctype.h>

#include "Bowser.h"
#include "ClientWindow.h"
#include "StringManip.h"
#include "IRCView.h"

struct URL 
{ 
		int32		offset; 
		BString		display, 
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

// We put settings here mostly to eliminate 
// the template crap through out all 
// the files that use a IRCView 

struct IRCViewSettings 
{ 
	BTextControl			*parentInput; 

	BFont					urlFont; 
	rgb_color				urlColor; 
	list<URL>				urls; 
}; 

IRCView::IRCView ( 
        BRect textframe, 
        BRect textrect, 
        BTextControl *inputControl) 

        : BTextView ( 
                textframe, 
                "IRCView", 
                textrect, 
                B_FOLLOW_ALL_SIDES, 
                B_WILL_DRAW | B_FRAME_EVENTS) 
{ 
        settings = new IRCViewSettings; 

        settings->parentInput = inputControl; 
        settings->urlFont  = *(bowser_app->GetClientFont (F_URL)); 
        settings->urlColor = bowser_app->GetColor (C_URL); 

        MakeEditable (false); 
        MakeSelectable (true); 
        SetStylable (true); 
} 

IRCView::~IRCView() 
{ 
        delete settings; 
} 

void 
IRCView::MouseDown (BPoint myPoint) 
{ 
        int32 start, finish; 

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
        BString url; 
        BString data (cData); 
        bool scrolling (static_cast<ClientWindow *>(Window())->Scrolling()); 

        // Previously calling SetFontAndColor is really the wrong approach, 
        // since it does not add to the run array.  You have to pass a run array 
        // for this to work.  Otherwise, things like selecting the text 
        // will force the text to redraw in the single run array (you'll lose 
        // all your coloring) 

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

                // woulda, coulda used a deque.. 
                // too many warnings on signed/ 
                // unsigned comparisons -- list 
                // works fine though 
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
                parent->ScrollRange(&scrollMin, &scrollMax); 
                
                float scrollVal = parent->ScrollPos(); 
                int32 curLine = (int32) ((scrollMax/LineHeight()) * (scrollVal/scrollMax)); 
                
                Delete (0,bytes); 
                if (!scrolling) 
                        parent->SetScrollPos(TextHeight(0, curLine));           
        } 

        if (scrolling) 
        { 
                if (TextLength() > 0) 
                ScrollToOffset (TextLength()); 
        } 
        return TextLength(); 
} 

int32 IRCView::URLLength (const char *outTemp) 
{ 
        int32 x = 0; 

        while (outTemp[x] 
        &&    (isdigit (outTemp[x])             // do these first! 
        ||     isalpha (outTemp[x]) 
        ||     outTemp[x] == '.' 
        ||     outTemp[x] == ',' 
        ||     outTemp[x] == '-' 
        ||     outTemp[x] == '/' 
        ||     outTemp[x] == ':' 
        ||     outTemp[x] == '~' 
        ||     outTemp[x] == '%' 
        ||     outTemp[x] == '+' 
        ||     outTemp[x] == '&' 
        ||     outTemp[x] == '_' 
        ||     outTemp[x] == '?' 
        ||     outTemp[x] == '=')) 
                ++x; 

        return x; 
} 

int32 IRCView::FirstMarker (const char *cData) 
{ 
        BString data (cData); 
        int32 urlMarkers[6]; 
        int32 marker (data.Length()); 
        int32 pos (0); 

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

void IRCView::ClearView() 
{ 
        SetText(""); 
        ScrollToOffset(0); 
} 

void 
IRCView::FrameResized (float width, float height) 
{ 
        BRect textrect = TextRect(); 

        textrect.right  = textrect.left + width - 7; 
        textrect.bottom = textrect.top + height - 1; 
        SetTextRect(textrect); 
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