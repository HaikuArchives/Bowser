
//#include <View.h>
#include <MenuBar.h>
//#include <Menu.h>
#include <MenuItem.h>
#include <ListView.h>
#include <ScrollView.h>
//#include <ListItem.h>

//#include <stdio.h>

#include "IRCDefines.h"
#include "Prompt.h"
#include "StatusView.h"
#include "Settings.h"
#include "Bowser.h"
#include "StringManip.h"
#include "ListWindow.h"

class ChannelItem : public BListItem
{
	BString					channel,
								users,
								topic;
	public:

								ChannelItem (
									const char *,
									const char *,
									const char *);
	virtual					~ChannelItem (void);

	const char 				*Channel (void) const;
	const char				*Users (void) const;
	const char				*Topic (void) const;

	virtual void			DrawItem (BView *, BRect, bool = false);
};

ListWindow::ListWindow (
	BRect frame,
	const char *title)
	: BWindow (
		frame,
		title,
		B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS
			| (bowser_app->GetWindowFollowsState()
				?  (B_NOT_ANCHORED_ON_ACTIVATE | B_NO_WORKSPACE_ACTIVATION)
				:  0)),

		filter (""),
		find (""),
		processing (false),
		channelWidth (0.0)
{
	frame = Bounds();

	BMenuBar *bar (new BMenuBar (frame, "menubar"));

	BMenu *menu (new BMenu ("Channel"));

	menu->AddItem (mFind = new BMenuItem (
		"Find" B_UTF8_ELLIPSIS, 
		new BMessage (M_LIST_FIND),
		'F'));
	menu->AddItem (mFindAgain = new BMenuItem (
		"Find Next", 
		new BMessage (M_LIST_FAGAIN),
		'G'));

	bar->AddItem (menu);

	menu = new BMenu ("View");
	menu->AddItem (mChannelSort = new BMenuItem (
		"Channel Sort",
		new BMessage (M_SORT_CHANNEL),
		'C'));
	mChannelSort->SetMarked (true);
	menu->AddItem (mUserSort    = new BMenuItem (
		"User Sort",
		new BMessage (M_SORT_USER),
		'U'));
	menu->AddSeparatorItem();
	menu->AddItem (mFilter = new BMenuItem (
		"Filter" B_UTF8_ELLIPSIS,
		new BMessage (M_FILTER_LIST),
		'L'));

	mChannelSort->SetMarked (true);

	bar->AddItem (menu);

	AddChild (bar);

	frame.top = bar->Frame().bottom + 1;
	BView *bgView (new BView (
		frame,
		"background",
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW));

	bgView->SetViewColor (212, 212, 212, 255);
	AddChild (bgView);

	status = new StatusView (bgView->Bounds());
	status->AddItem (new StatusItem (
		"Count:", "@@@@@"),
		true);
	status->AddItem (new StatusItem (
		"Status:", "@@@@@@@@@@@@"),
		true);
	status->AddItem (new StatusItem (
		"Filter:",
		"@@@@@@@@@@@@@@@@@@@@@@",
		STATUS_ALIGN_LEFT),
		true);
	status->SetItemValue (2, filter.String());

	bgView->AddChild (status);

	frame = bgView->Bounds().InsetByCopy (1, 1);

	listView = new BListView (
		BRect (
			frame.left,
			frame.top,
			frame.right - B_V_SCROLL_BAR_WIDTH,
			status->Frame().top - B_H_SCROLL_BAR_HEIGHT - 1),
		"list",
		B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);

	listView->SetInvocationMessage (new BMessage (M_LIST_INVOKE));

	scroller = new BScrollView (
		"scroller",
		listView,
		B_FOLLOW_ALL_SIDES,
		0,
		true,
		true,
		B_PLAIN_BORDER);

	bgView->AddChild (scroller);
	BScrollBar *sBar (scroller->ScrollBar (B_HORIZONTAL));
	sBar->SetRange (0.0, 0.0);
	sBar->SetValue (0);
	listView->MakeFocus (true);

	memset (&re, 0, sizeof (re));
	memset (&fre, 0, sizeof (fre));

	settings = new WindowSettings (
		this,
		title,
		BOWSER_SETTINGS);
	settings->Restore();
}

ListWindow::~ListWindow (void)
{
	BListItem *item;

	showing.MakeEmpty();
	listView->MakeEmpty();
	#ifdef __x86_64__
	while ((item = (BListItem *)list.RemoveItem (0)) != 0)
	#else
	while ((item = (BListItem *)list.RemoveItem (0L)) != 0)
	#endif
	{
		delete item;
	}

	regfree (&re);
	regfree (&fre);
	delete settings;
}

bool
ListWindow::QuitRequested (void)
{
	BMessage aMsg (M_LIST_SHUTDOWN);
	BString serverName (GetWord (Title(), 2));

	aMsg.AddString ("server", serverName.String());
	bowser_app->PostMessage (&aMsg);
	settings->Save();
	
	return true;
}

void
ListWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_LIST_COMMAND:

			if (!processing)
			{
				BMessage sMsg (M_SERVER_SEND);
				BString serverName (GetWord (Title(), 2));

				sMsg.AddString ("data", "LIST");
				sMsg.AddString ("server", serverName.String());

				bowser_app->PostMessage (&sMsg);
				processing = true;
				mFind->SetEnabled (false);
				mFindAgain->SetEnabled (false);
				mChannelSort->SetEnabled (false);
				mUserSort->SetEnabled (false);
				mFilter->SetEnabled (false);

				BListItem *item;
				showing.MakeEmpty();
				listView->MakeEmpty();
				#ifdef __x86_64__
				while ((item = (BListItem *)list.RemoveItem (0)) != 0)
				#else
				while ((item = (BListItem *)list.RemoveItem (0L)) != 0)
				#endif
				{
					delete item;
				}
				status->SetItemValue (0, "0");
			}
			break;

		case M_LIST_BEGIN:

			status->SetItemValue (1, "Loading");
			channelWidth = 0.0;
			break;

		case M_LIST_DONE:
		{
			status->SetItemValue (1, "Sorting");
			UpdateIfNeeded();

			if (filter.Length())
			{
				showing.MakeEmpty();
				
				sTopicWidth = sChannelWidth = 0.0;
				for (int32 i = 0; i < list.CountItems(); ++i)
				{
					ChannelItem *item ((ChannelItem *)list.ItemAt (i));

					if (regexec (&re, item->Channel(), 0, 0, 0) != REG_NOMATCH)
					{
						float width;

						showing.AddItem (item);

						width = listView->StringWidth (item->Channel());
						if (width > sChannelWidth)
						{
							sChannelWidth = width;
						}
						
						width = listView->StringWidth (item->Topic());
						if (width > sTopicWidth)
						{
							sTopicWidth = width;
						}
					}
				}
			}
			else
			{
				showing = list;

				sChannelWidth = channelWidth;
				sTopicWidth = topicWidth;
			}

			sLineWidth = ChannelWidth() 
				+ listView->StringWidth ("@@@@")
				+ 14.0 + sTopicWidth;

			BScrollBar *bar (scroller->ScrollBar (B_HORIZONTAL));
			bar->SetRange (0.0, sLineWidth - listView->Frame().Width());

			float low (min_c (Frame().Width() / sLineWidth, 1.0));

			if (low == 1.0)
			{
				bar->SetRange (0.0, 0.0);
			}
			else
			{
				bar->SetProportion (low);
			}
			
			showing.SortItems (SortChannels);
			status->SetItemValue (1, "Adding");
			UpdateIfNeeded();

			listView->AddList (&showing);
			status->SetItemValue (1, "Done");
			mFind->SetEnabled (true);
			mFindAgain->SetEnabled (true);
			mChannelSort->SetEnabled (true);
			mUserSort->SetEnabled (true);
			mFilter->SetEnabled (true);
			processing = false;

			BString cString;
			cString << showing.CountItems();
			status->SetItemValue (0, cString.String());
			break;
		}

		case M_LIST_EVENT:
		{
			const char *channel, *users, *topic;

			msg->FindString ("channel", &channel);
			msg->FindString ("users", &users);
			msg->FindString ("topic", &topic);

			list.AddItem (new ChannelItem (channel, users, topic));

			float width (listView->StringWidth (channel));

			if (width > channelWidth)
			{
				channelWidth = width;
			}
			
			width = listView->StringWidth (topic);
			if (width > topicWidth)
			{
				topicWidth = width;
			}
				
			BString countStr;
			countStr << list.CountItems();
			status->SetItemValue (0, countStr.String());

			break;
		}

		case M_SORT_CHANNEL:
		case M_SORT_USER:

			mFind->SetEnabled (false);
			mFindAgain->SetEnabled (false);
			mChannelSort->SetEnabled (false);
			mUserSort->SetEnabled (false);
			mFilter->SetEnabled (false);
			listView->MakeEmpty();

			status->SetItemValue (1, "Sorting");
			UpdateIfNeeded();

			showing.SortItems (
				msg->what == M_SORT_CHANNEL
				? SortChannels : SortUsers);
			status->SetItemValue (1, "Adding");
			UpdateIfNeeded();

			listView->AddList (&showing);
			status->SetItemValue (1, "Done");

			mFind->SetEnabled (true);
			mFindAgain->SetEnabled (true);
			mChannelSort->SetEnabled (true);
			mUserSort->SetEnabled (true);
			mFilter->SetEnabled (true);


			mChannelSort->SetMarked (msg->what == M_SORT_CHANNEL);
			mUserSort->SetMarked (msg->what == M_SORT_USER);
			break;

		case M_FILTER_LIST:
			
			if (msg->HasString ("text"))
			{
				const char *buffer;

				msg->FindString ("text", &buffer);

				if (filter != buffer)
				{
					filter = buffer;

					status->SetItemValue (2, filter.String());

					regfree (&re);
					memset (&re, 0, sizeof (re));
					regcomp (
						&re,
						filter.String(),
						REG_EXTENDED | REG_ICASE | REG_NOSUB);
					listView->MakeEmpty();

					BScrollBar *bar (scroller->ScrollBar (B_HORIZONTAL));
					bar->SetRange (0.0, 0.0);
					PostMessage (M_LIST_DONE);
					processing = true;
				}
			}
			else
			{
				PromptWindow *prompt (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"  Filter:",
					"List Filter",
					filter.String(),
					this,
					new BMessage (M_FILTER_LIST),
					new RegExValidate ("Filter"),
					true));
				prompt->Show();
			}
			break;

		case M_LIST_FIND:

			if (msg->HasString ("text"))
			{
				int32 selection (listView->CurrentSelection());
				const char *buffer;

				msg->FindString ("text", &buffer);

				if (strlen (buffer) == 0)
				{
					find = buffer;
					break;
				}

				if (selection < 0)
				{
					selection = 0;
				}
				else
				{
					++selection;
				}

				if (find != buffer)
				{
					regfree (&fre);
					memset (&fre, 0, sizeof (fre));
					regcomp (
						&fre,
						buffer,
						REG_EXTENDED | REG_ICASE | REG_NOSUB);
					find = buffer;
				}

				ChannelItem *item;
				int32 i;

				for (i = selection; i < listView->CountItems(); ++i)
				{
					item = (ChannelItem *)listView->ItemAt (i);

					if (regexec (&fre, item->Channel(), 0, 0, 0) != REG_NOMATCH)
						break;
				}

				if (i < listView->CountItems())
				{
					listView->Select (i);
					listView->ScrollToSelection();
				}
				else
				{
					listView->DeselectAll();
				}
			}
			else
			{
				PromptWindow *prompt (new PromptWindow (
					BPoint (Frame().right - 100, Frame().top + 50),
					"    Find:",
					"Find",
					find.String(),
					this,
					new BMessage (M_LIST_FIND),
					new RegExValidate ("Find:"),
					true));
				prompt->Show();
			}
			break;

		case M_LIST_FAGAIN:
			if (find.Length())
			{
				msg->AddString ("text", find.String());
				msg->what = M_LIST_FIND;
				PostMessage (msg);
			}
			break;

		case M_LIST_INVOKE:
		{
			int32 index;

			msg->FindInt32 ("index", &index);

			if (index >= 0)
			{
				ChannelItem *item ((ChannelItem *)listView->ItemAt (index));
				BString serverName (GetWord (Title(), 2));
				BMessage msg (M_SUBMIT);
				BString buffer;
				
				buffer << "/JOIN " << item->Channel();

				msg.AddBool ("history", false);
				msg.AddBool ("clear", false);
				msg.AddString ("input", buffer.String());
				msg.AddString ("server", serverName.String());
				bowser_app->PostMessage (&msg);
			}

			break;
		}

		case M_STATE_CHANGE:

			if (msg->HasBool ("windowfollows"))
			{
				bool windowfollows;

				msg->FindBool ("windowfollows", &windowfollows);

				if (windowfollows)
				{
					SetFlags (Flags()
						| B_NOT_ANCHORED_ON_ACTIVATE
						| B_NO_WORKSPACE_ACTIVATION);
				}
				else
				{
					SetFlags (Flags()
						& ~(B_NOT_ANCHORED_ON_ACTIVATE
						|   B_NO_WORKSPACE_ACTIVATION));
				}
			}
			break;

		default:
			BWindow::MessageReceived (msg);
	}
}

void
ListWindow::FrameResized (float width, float)
{
	if (!processing)
	{
		sLineWidth = ChannelWidth() 
			+ listView->StringWidth ("@@@@")
			+ 14.0 + sTopicWidth;

		BScrollBar *bar (scroller->ScrollBar (B_HORIZONTAL));
		bar->SetRange (0.0, sLineWidth - listView->Frame().Width());

		float low (min_c (width / sLineWidth, 1.0));

		if (low == 1.0)
		{
			bar->SetRange (0.0, 0.0);
		}
		else
		{
			bar->SetProportion (low);
		}
	
		bar->SetSteps (5.0, listView->Frame().Width());
	}
}

int
ListWindow::SortChannels (const void *arg1, const void *arg2)
{
	const ChannelItem
		**firstItem ((const ChannelItem **)arg1),
		**secondItem ((const ChannelItem **)arg2);

	return strcasecmp ((*firstItem)->Channel(), (*secondItem)->Channel());
}

int
ListWindow::SortUsers (const void *arg1, const void *arg2)
{
	const ChannelItem
		**firstItem ((const ChannelItem **)arg1),
		**secondItem ((const ChannelItem **)arg2);
	BString users[2];

	users[0] = (*firstItem)->Users();
	users[1] = (*secondItem)->Users();

	users[0].Prepend ('0', 10 - users[0].Length());
	users[1].Prepend ('0', 10 - users[1].Length());

	users[0] << (*firstItem)->Channel();
	users[1] << (*secondItem)->Channel();

	return users[0].ICompare (users[1]);
}

float
ListWindow::ChannelWidth (void) const
{
	return sChannelWidth > Frame().Width() / 2
		? floor (listView->Frame().Width() / 2)
		: sChannelWidth;
}

ChannelItem::ChannelItem (
	const char *channel_,
	const char *users_,
	const char *topic_)

	: BListItem (),
	  channel (channel_),
	  users (users_),
	  topic (topic_)
{
}

ChannelItem::~ChannelItem (void)
{
}

const char *
ChannelItem::Channel (void) const
{
	return channel.String();
}

const char *
ChannelItem::Users (void) const
{
	return users.String();
}

const char *
ChannelItem::Topic (void) const
{
	return topic.String();
}

void
ChannelItem::DrawItem (BView *owner, BRect frame, bool)
{
	ListWindow *listWindow ((ListWindow *)owner->Window());

	if (IsSelected())
	{
		owner->SetLowColor (180, 180, 180, 255);
		owner->FillRect (frame, B_SOLID_LOW);
	}
	else //if (complete)
	{
		owner->SetLowColor (255, 255, 255, 255);
		owner->FillRect (frame, B_SOLID_LOW);
	}

	float channelWidth (listWindow->ChannelWidth());
	BFont font;
	font_height fh;

	owner->GetFontHeight (&fh);
	owner->GetFont (&font);

	float cWidth (font.StringWidth (channel.String()));


	if (cWidth > channelWidth)
	{
		const char *inputs[1];
		char *outputs[1];

		inputs[0]  = channel.String();
		outputs[0] = new char [channel.Length() + 5];
		font.GetTruncatedStrings (
			inputs,
			1,
			B_TRUNCATE_END,
			channelWidth,
			outputs);

		owner->SetHighColor (0, 0, 0, 255);
		owner->SetDrawingMode (B_OP_OVER);

		owner->DrawString (
			outputs[0], 
			BPoint (4, frame.bottom - fh.descent));

		delete [] outputs[0];
	}
	else
	{
		owner->DrawString (
			channel.String(),
			BPoint (4, frame.bottom - fh.descent));
	}

	float width (font.StringWidth ("@@@@"));
	owner->DrawString (
		users.String(),
		BPoint (
			channelWidth
				+ width 
				- font.StringWidth (users.String()),
			frame.bottom - fh.descent));

	owner->DrawString (
		topic.String(),
		BPoint (
			channelWidth + width + 10,
			frame.bottom - fh.descent));

	owner->SetDrawingMode (B_OP_COPY);
}

