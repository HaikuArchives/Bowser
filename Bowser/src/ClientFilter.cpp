
#include <Alert.h>
//#include <File.h>
#include <NodeInfo.h>
#include <TextControl.h>
//#include <TextView.h>
//#include <Clipboard.h>
#include <Roster.h>
#include <ScrollView.h>

//#include <stdio.h>
//#include <string.h>

#include "IRCView.h"
#include "IRCDefines.h"
#include "ClientWindow.h"
#include "ClientFilter.h"

ClientInputFilter::ClientInputFilter (ClientWindow *win)
	: BMessageFilter (B_ANY_DELIVERY, B_ANY_SOURCE),
	  window (win),
	  handledDrop (false)
{
}

ClientInputFilter::~ClientInputFilter (void)
{
}

filter_result
ClientInputFilter::Filter (BMessage *msg, BHandler **)
{
	filter_result result (B_DISPATCH_MESSAGE);
	switch (msg->what)
	{
		case B_MOUSE_MOVED:
		case B_KEY_UP:
			break;

		case B_KEY_DOWN:
		{
			result = HandleKeys (msg);
			break;
		}

		case B_MOUSE_UP:
			if (handledDrop)
			{
				handledDrop = false;
				result = B_SKIP_MESSAGE;
			}
			break;

		case B_MIME_TYPE:
			if (msg->HasData ("text/plain", B_MIME_TYPE))
			{
				const char *buffer;
				ssize_t size;

				msg->FindData (
					"text/plain",
					B_MIME_TYPE,
					0,
					reinterpret_cast<const void **>(&buffer),
					&size);

				// We copy it, because B_MIME_TYPE
				// might not be \0 terminated

				BString string;
				
				string.Append (buffer, size);
				HandleDrop (string.String());

				handledDrop = true;
				result = B_SKIP_MESSAGE;
			}
			break;
		
		case B_SIMPLE_DATA:
			if (msg->HasRef ("refs"))
			{
				for (int32 i = 0; msg->HasRef ("refs", i); ++i)
				{
					entry_ref ref;

					msg->FindRef ("refs", &ref);

					char mime[B_MIME_TYPE_LENGTH];
					BFile file (&ref, B_READ_ONLY);
					BNodeInfo info (&file);
					off_t size;

					if (file.InitCheck()               == B_NO_ERROR
					&&  file.GetSize (&size)           == B_NO_ERROR
					&&  info.InitCheck()               == B_NO_ERROR
					&&  info.GetType (mime)            == B_NO_ERROR
					&&  strncasecmp (mime, "text/", 5) == 0)
					{
						char *buffer (new char [size + 1]);

						if (buffer)
						{
							// Oh baby!
							file.Read (buffer, size);
							buffer[size] = 0;

							HandleDrop (buffer);
							delete [] buffer;
							break;
						}
					}
				}

				// Give the window a chance to handle non
				// text files.  If it's a message window, it'll
				// kick off a dcc send
				window->DroppedFile (msg);
			}
			break;
		
		default:
		{
			//printf ("FILTER UNHANDLED: ");
			//msg->PrintToStream();
			break;
		}
	}

	return result;
}

filter_result
ClientInputFilter::HandleKeys (BMessage *msg)
{
	filter_result result (B_DISPATCH_MESSAGE);
	int8 keyStroke;
	int32 modifiers;

	msg->FindInt8 ("byte", &keyStroke);
	msg->FindInt32 ("modifiers", &modifiers);

	// Must check modifiers -- things like control-tab must work!
	if ((modifiers & B_SHIFT_KEY)   == 0
	&&  (modifiers & B_OPTION_KEY)  == 0
	&&  (modifiers & B_COMMAND_KEY) == 0
	&&  (modifiers & B_CONTROL_KEY) == 0)
		switch(keyStroke)
		{
			case B_UP_ARROW:
				window->PostMessage (M_PREVIOUS_INPUT);
				result = B_SKIP_MESSAGE;
				break;
	
			case B_DOWN_ARROW:
				window->PostMessage (M_NEXT_INPUT);
				result = B_SKIP_MESSAGE;
				break;
	
			case B_RETURN:

				if (window->input->TextView()->TextLength())
				{
					BMessage msg (M_SUBMIT);

					msg.AddString (
						"input",
						window->input->TextView()->Text());
					window->PostMessage (&msg);
				}

				result = B_SKIP_MESSAGE;
				break;
	
			case '\t': // tab, skip
			{
				window->TabExpansion();
				result = B_SKIP_MESSAGE;
				break;
			}
		}
	if (modifiers & B_COMMAND_KEY)
	{
		switch (keyStroke)
		{
			case 'v':
			case 'V':
			{
				BClipboard clipboard("system");
				const char *text;
				int32 textLen;
				BMessage *clip = (BMessage *)NULL;
				if (clipboard.Lock()) {
				   if ((clip = clipboard.Data()))
	    			if (clip->FindData("text/plain", B_MIME_TYPE, 
	    				(const void **)&text, &textLen) != B_OK)
	 				{
	 	  				clipboard.Unlock();
	 					break;
	    			}
	    		}
	   			clipboard.Unlock();
	   			BString data(text, textLen);
	   			HandleDrop(data.String());
	   			result = B_SKIP_MESSAGE;
	   			break;
			}
			case B_UP_ARROW:
			{
				if ( window->scroll->ScrollBar(B_VERTICAL)->Value() != 0)
				{
					window->text->ScrollBy(0.0, window->text->LineHeight() * -1);
					result = B_SKIP_MESSAGE;
				}
				break;
			}
			
			case B_DOWN_ARROW:
			{
				float min, max;
				window->scroll->ScrollBar(B_VERTICAL)->GetRange(&min, &max);
				if ( window->scroll->ScrollBar(B_VERTICAL)->Value() != max)
				{ 
					window->text->ScrollBy(0.0, window->text->LineHeight());
					result = B_SKIP_MESSAGE;
				}
				break;
			}
			
			case B_HOME:
			{
				window->text->ScrollTo(0.0, 0.0);
				result = B_SKIP_MESSAGE;
				break;
			}
			
			case B_END:
			{
				float min, max;
				window->scroll->ScrollBar(B_VERTICAL)->GetRange(&min, &max);
				window->text->ScrollTo(0.0, max);
				result = B_SKIP_MESSAGE;
				break;
			}
			
			case B_PAGE_UP:
			{
				BRect myrect (window->text->Bounds());
				float height = myrect.bottom - myrect.top;
				
				if ( window->scroll->ScrollBar(B_VERTICAL)->Value() > height)
				{
					window->text->ScrollBy(0.0, 
						-1 * height);
				}
				else
					window->text->ScrollTo(0.0, 0.0);
				result = B_SKIP_MESSAGE;
				break;
			}
			
			case B_PAGE_DOWN:
			{
				BRect myrect (window->text->Bounds());
				float height = myrect.bottom - myrect.top;
			
				float min, max;
				window->scroll->ScrollBar(B_VERTICAL)->GetRange(&min, &max);
				if ( window->scroll->ScrollBar(B_VERTICAL)->Value() != max)
					window->text->ScrollBy(0.0, height);
				result = B_SKIP_MESSAGE;
				break;
			}
		}
	}
	return result;
}
	
void
ClientInputFilter::HandleDrop (const char *buffer)
{
	BMessage msg (M_SUBMIT_RAW);
	const char *place;
	int32 lines (0);

	while ((place = strchr (buffer, '\n')))
	{
		BString str;
				
		str.Append (buffer, place - buffer);
		msg.AddString ("data", str.String());
		++lines;
		buffer = place + 1;
	}

	if (*buffer)
	{
		msg.AddString ("data", buffer);
		++lines;
	}

	int32 result (0);

	if (lines > 1)
	{
		BString str;

		str << "As if there isn't enough, you ";
		str << "are about to add " << lines;
		str << " more lines of spam to ";
		str << "the internet.  How ";
		str << "would you like to go about this?";

		BAlert *alert (new BAlert (
			"Spam",
			str.String(),
			"Cancel",
			"Spam!",
			"Single Line",
			B_WIDTH_FROM_WIDEST,
			B_OFFSET_SPACING,
			B_WARNING_ALERT));

		result = alert->Go();
	}
	
	int32 start, finish;
	window->input->TextView()->GetSelection (&start, &finish);
	msg.AddInt32 ("selstart", start);
	msg.AddInt32 ("selend", finish);

	if (result || lines == 1)
	{
		msg.AddBool ("lines", result == 1);
		window->PostMessage (&msg);
		window->input->MakeFocus(false);
		window->input->MakeFocus(true);
	}

}

