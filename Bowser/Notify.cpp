
#define NOTIFY_PULSE_RATE				400000

#include <Application.h>
#include <AppFileInfo.h>
#include <File.h>
#include <Entry.h>
#include <Path.h>
#include <Bitmap.h>
#include <Window.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <List.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <Screen.h>

#include "IRCDefines.h"
#include "Notify.h"

#include <stdio.h>

struct NotifyData
{
	map<BString, NotifyData *>			clients;				// Yeah, I know this is wasted
																		// on the second level deep..
																		// shoot me!

	BString									name;
	int32										sid;

	bool										active,
												news,
												nick,
												other;

	BMessenger								msgr;
};

class NotifyPopUpMenu : public BPopUpMenu
{
	const map<int32, NotifyData *>	&servers;
	BRect										vFrame,
												wFrame;

	public:
												NotifyPopUpMenu (
													const map<int32, NotifyData *> &,
													BRect,
													BRect);

	virtual 									~NotifyPopUpMenu (void);
	virtual BPoint							ScreenLocation (void);
};

class NotifyMenuItem : public BMenuItem
{
	bool										nick,
												news,
												other;

	public:

												NotifyMenuItem (
													const char *,
													BMessage *,
													bool,
													bool,
													bool);
	virtual									~NotifyMenuItem (void);
	virtual void							DrawContent (void);
};

static rgb_color NickColor   = {122,   0,   0, 255};
static rgb_color NewsColor   = {122, 122,   0, 255};
static rgb_color OtherColor  = {  0,   0, 122, 255};

NotifyView::NotifyView (int32 mask_)
	: BView (
		BRect (0, 9, 0, 9),
		"Bowser:Notify",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW),

		newsOn (false),
		nickOn (false),
		otherOn (false),
		news (0),
		nicks (0),
		others (0),
		mask (mask_),
		runner (0)
{
	app_info info;

	be_app->GetAppInfo (&info);
	signature = info.signature;
	app_ref   = info.ref;
	bitmap[0] = bitmap[1] = bitmap[2] = bitmap[3] = 0;

	#ifdef DEV_BUILD
	if (NotifyStatus)
	{
		printf ("*** Creating Bowser copy of Notify\n");
		printf ("*** App signature %s\n", info.signature);


		BEntry entry (&info.ref);
		BPath path;

		entry.GetPath (&path);
		printf ("*** App location %s\n", path.Path());
	}
	#endif
}

NotifyView::NotifyView (BMessage *archive)
	: BView (archive),

		newsOn (false),
		nickOn (false),
		otherOn (false),
		news (0),
		nicks (0),
		others (0),
		mask (0),
		runner (0)
{
	const char *buffer;
	status_t status;

	bitmap[0] = bitmap[1] = bitmap[2] = bitmap[3] = 0;
	#ifdef DEV_BUILD
	bool notifystatus;

	printf ("*** Notify is Instantiating from Archive\n");
	#endif

	if ((status = archive->FindRef ("app_ref", &app_ref))    != B_NO_ERROR
	||  (status = archive->FindString ("add_on", &buffer))   != B_NO_ERROR
	||  (status = archive->FindInt32 ("notify:mask", &mask)) != B_NO_ERROR
	
	#ifdef DEV_BUILD
	||  (status = archive->FindBool ("notify:status", &notifystatus)) != B_NO_ERROR
	#endif

	)
	{
		#ifdef DEV_BUILD
		if (notifystatus)
			printf ("*** Had trouble unarchiving: %s\n", strerror (status));
		#endif
		;
	}

	signature = buffer;
}

NotifyView *
NotifyView::Instantiate (BMessage *archive)
{
	if (validate_instantiation (archive, "NotifyView"))
		return new NotifyView (archive);

	return 0;
}

NotifyView::~NotifyView (void)
{
	// What the hey.. just in case
	for (int32 i = 0; i < 4; ++i)
		if (bitmap[i]) delete bitmap[i];

	if (runner)
		delete runner;
}

status_t
NotifyView::Archive (BMessage *archive, bool deep) const
{
	status_t status;

	status = BView::Archive (archive, deep);

	#ifdef DEV_BUILD
	if (NotifyStatus)
		printf ("*** Archiving Bowser copy of Notify\n");

	if (status != B_NO_ERROR)
	{
		printf ("*** Could not archive BView of Notify: %s\n",
			strerror (status));
	}
	#endif

	// System stuff
	if ((status = archive->AddString ("add_on", signature.String())) != B_NO_ERROR
	||  (status = archive->AddBool ("be:unload_on_delete", true))    != B_NO_ERROR
	||  (status = archive->AddRef ("app_ref", &app_ref))             != B_NO_ERROR
	||  (status = archive->AddInt32 ("notify:mask", mask))           != B_NO_ERROR

	#ifdef DEV_BUILD
	||  (status = archive->AddBool ("notify:status", NotifyStatus))  != B_NO_ERROR
	#endif
	)

	{
		#ifdef DEV_BUILD
		if (NotifyStatus)
			printf ("*** Could not archive elements for Notify: %s\n", strerror (status));
		#endif
		;
	}

	return B_NO_ERROR;
}

void
NotifyView::Draw (BRect frame)
{
	SetDrawingMode (B_OP_ALPHA);
	SetBlendingMode (B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	// Again, remember folks...
	// Nicks take precedence
	if (bitmap[1] && nicks && nickOn)
	{
		DrawBitmap (bitmap[1], BPoint (0, 0));
	}
	else if (bitmap[3] && !nicks && others && otherOn)
	{
		DrawBitmap (bitmap[3], BPoint (0, 0));
	}
	else if (bitmap[2] && !nicks && !others && news && newsOn)
	{
		DrawBitmap (bitmap[2], BPoint (0, 0));
	}
	else if (bitmap[0])
	{
		DrawBitmap (bitmap[0], BPoint (0, 0));
	}
}

void
NotifyView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NEW_CLIENT:
		{
			const char *id;

			msg->FindString ("id", &id);
			NotifyData *nData (new NotifyData);

			nData->name   = id;

			msg->FindInt32 ("sid", &nData->sid);
			msg->FindMessenger ("msgr", &nData->msgr);

			nData->active = true;
			nData->news   = false;
			nData->nick   = false;
			nData->other  = false;

			if (servers.find (nData->sid) == servers.end())
				// brand new server
				servers[nData->sid] = nData;
			else
				servers[nData->sid]->clients[nData->name] = nData;
				
			break;
		}

		case M_ACTIVATION_CHANGE:
		{
			NotifyData *nData;
			const char *id;
			int32 sid;
			bool active;

			msg->FindString ("id", &id);
			msg->FindInt32 ("sid", &sid);
			msg->FindBool ("active", &active);

			if (servers[sid]->name == id)
				// the server has activation
				nData = servers[sid];
			else
				nData = servers[sid]->clients[id];

			if (!nData->active && active)
			{
				if (nData->news)
					--news;

				if (nData->nick)
					--nicks;

				if (nData->other)
					--others;
			}

			nData->active = active;
			nData->news   = false;
			nData->nick   = false;
			nData->other  = false;
			break;
		}

		case M_ID_CHANGE:
		{
			const char *id;
			int32 sid;

			msg->FindString ("id", &id);
			msg->FindInt32 ("sid", &sid);

			map<BString, NotifyData *>::iterator it;

			BString strId (id);
			it = servers[sid]->clients.find (strId);

			if (it != servers[sid]->clients.end())
			{
				NotifyData *nData;
				const char *newid;

				msg->FindString ("newid", &newid);
				nData = it->second;

				printf ("NOTIFY Changing id from %s to %s\n", it->first.String(), newid);
				strId = newid;
				servers[sid]->clients.erase (it);
				servers[sid]->clients[strId] = nData;
				nData->name = strId;
			}

			break;
		}

		case M_QUIT_CLIENT:
		{
			map<BString, NotifyData *>::iterator it;
			const char *id;
			int32 sid;

			msg->FindString ("id", &id);
			msg->FindInt32 ("sid", &sid);

			// Removing server
			if (servers[sid]->name == id)
			{
				while (!servers[sid]->clients.empty())
				{
					it = servers[sid]->clients.begin();

					if (it->second->news)  --news;
					if (it->second->nick)  --nicks;
					if (it->second->other) --others;

					delete it->second;
					servers[sid]->clients.erase (it);
				}

				map<int32, NotifyData *>::iterator sit;

				sit = servers.find (sid);

				if (sit->second->news)  --news;
				if (sit->second->nick)  --nicks;
				if (sit->second->other) --others;
				delete sit->second;
				servers.erase (sit);
			}
			else
			{
				it = servers[sid]->clients.find (id);

				if (it->second->news)  --news;
				if (it->second->nick)  --nicks;
				if (it->second->other) --others;
				delete it->second;
				servers[sid]->clients.erase (it);
			}
			
			break;
		}
		
		case M_NEWS_CLIENT:

			if ((mask & NOTIFY_CONT_BIT) != 0)
			{
				NotifyData *nData;
				const char *id;
				int32 sid;
	
				msg->FindString ("id", &id);
				msg->FindInt32 ("sid", &sid);
	
				if (servers[sid]->name == id)
					nData = servers[sid];
				else
					nData = servers[sid]->clients[id];
	
				if (!nData->active && !nData->news)
				{
					++news;
					nData->news = true;
				}
			}
			break;

		case M_NICK_CLIENT:

			if ((mask & NOTIFY_NICK_BIT) != 0)
			{
				NotifyData *nData;
				const char *id;
				int32 sid;
				bool me;

				msg->FindString ("id", &id);
				msg->FindInt32 ("sid", &sid);
				msg->FindBool ("me", &me);

				if (servers[sid]->name == id)
					nData = servers[sid];
				else
					nData = servers[sid]->clients[id];

				if (!nData->active && me && !nData->nick)
				{
					++nicks;
					nData->nick = true;
				}
				else if (!nData->active && !me && !nData->other)
				{
					++others;
					nData->other = true;
				}
			}
			break;

		case M_NOTIFY_SELECT:
		{
			BMessenger msgr;

			msg->FindMessenger ("msgr", &msgr);

			if (msgr.IsValid())
				msgr.SendMessage (M_NOTIFY_SELECT);
			break;
		}

		case M_NOTIFY_PULSE:

			if ((nicks && (mask & NOTIFY_NICK_FLASH_BIT) != 0)
			||  (nicks && (mask & NOTIFY_NICK_FLASH_BIT) == 0 && !nickOn)
			|| (!nicks && nickOn))
			{
				nickOn = nickOn ? false : true;
				Draw (Bounds());
			}
			else if ((others && (mask & NOTIFY_OTHER_FLASH_BIT) != 0)
			||       (others && (mask & NOTIFY_OTHER_FLASH_BIT) && !otherOn)
			||      (!others && otherOn))
			{
				otherOn = otherOn ? false : true;
				Draw (Bounds());
			}
			else if ((news && (mask & NOTIFY_CONT_FLASH_BIT) != 0)
			||       (news && (mask & NOTIFY_CONT_FLASH_BIT) == 0 && !newsOn)
			||      (!news  && newsOn))
			{
				newsOn = newsOn ? false : true;
				Draw (Bounds());
			}
			break;

		default:
			BView::MessageReceived (msg);
	}
}

void
NotifyView::AttachedToWindow (void)
{
	BBitmap icon (
		BRect (0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
		B_CMAP8,
		false,
		false);
	BAppFileInfo info;
	BFile file;

	BView::AttachedToWindow();

	file.SetTo (&app_ref, B_READ_ONLY);
	info.SetTo (&file);

	if (info.GetIcon (&icon, B_MINI_ICON) == B_OK)
	{
		for (int32 i = 0; i < 4; ++i)
			bitmap[i] = new BBitmap (
				BRect (0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
				B_RGBA32,
				false,
				false);
		
		ResizeTo (
			bitmap[0]->Bounds().Width(),
			bitmap[0]->Bounds().Height());

		// We convert the bitmap to a 32 bitmap to
		// do the alpha drawing B_CMAP8 doesn't handle
		// transparency doesn't work with alpha drawing :(
		const color_map *cmap (BScreen().ColorMap());
		unsigned char *ibits ((unsigned char *)icon.Bits());
		unsigned char *bbits[4];
		const rgb_color background = Parent()->ViewColor();

		bbits[0] = (unsigned char *)bitmap[0]->Bits();
		bbits[1] = (unsigned char *)bitmap[1]->Bits();
		bbits[2] = (unsigned char *)bitmap[2]->Bits();
		bbits[3] = (unsigned char *)bitmap[3]->Bits();

		for (int32 i = 0; ibits + i < (unsigned char *)icon.Bits() + icon.BitsLength(); ++i)
		{
			int32 j (0);

			for (int32 k = 0; k < icon.BytesPerRow(); ++k)
			{
				if (ibits[k] == B_TRANSPARENT_MAGIC_CMAP8)
				{
					bbits[0][j + 0] = bbits[1][j + 0] = bbits[2][j + 0] = bbits[3][j + 0] = background.blue;
					bbits[0][j + 1] = bbits[1][j + 1] = bbits[2][j + 1] = bbits[3][j + 1] = background.green;
					bbits[0][j + 2] = bbits[1][j + 2] = bbits[2][j + 2] = bbits[3][j + 2] = background.red;
					bbits[0][j + 3] = bbits[1][j + 3] = bbits[2][j + 3] = bbits[3][j + 3] = 255;
				}
				else
				{
					unsigned char index (ibits[k]);

					bbits[0][j + 0] = cmap->color_list[index].blue;
					bbits[0][j + 1] = cmap->color_list[index].green;
					bbits[0][j + 2] = cmap->color_list[index].red;
					bbits[0][j + 3] = 255;

					bbits[1][j + 0] = cmap->color_list[index].blue;
					bbits[1][j + 1] = cmap->color_list[index].green;
					bbits[1][j + 2] = 255;
					bbits[1][j + 3] = 255;

					bbits[2][j + 0] = cmap->color_list[index].blue;
					bbits[2][j + 1] = min_c (cmap->color_list[index].green + 100L, 255L);
					bbits[2][j + 2] = min_c (cmap->color_list[index].red + 100L, 255L);
					bbits[2][j + 3] = 255;

					bbits[3][j + 0] = 255;
					bbits[3][j + 1] = cmap->color_list[index].green;
					bbits[3][j + 2] = cmap->color_list[index].red;
					bbits[3][j + 3] = 255;
				}
					
				j += 4;
			}

			ibits += icon.BytesPerRow();
			bbits[0] += bitmap[0]->BytesPerRow();
			bbits[1] += bitmap[1]->BytesPerRow();
			bbits[2] += bitmap[2]->BytesPerRow();
			bbits[3] += bitmap[3]->BytesPerRow();
		}

		runner = new BMessageRunner (
			BMessenger (this),
			new BMessage (M_NOTIFY_PULSE),
			NOTIFY_PULSE_RATE);
	}

	SetViewColor (Parent()->ViewColor());
}

void
NotifyView::DetachedFromWindow (void)
{
	BView::DetachedFromWindow();

	for (int32 i = 0; i < 4; ++i)
		if (bitmap[i])
		{
			delete bitmap[i];
			bitmap[i] = 0;
		}

	if (runner)
	{
		delete runner;
		runner = 0;
	}
}

void
NotifyView::MouseDown (BPoint point)
{
	BMessage *msg (Window()->CurrentMessage());
	uint32 buttons;
	uint32 modifiers;

	msg->FindInt32 ("buttons", (int32 *)&buttons);
	msg->FindInt32 ("modifiers", (int32 *)&modifiers);

	if (buttons   == B_PRIMARY_MOUSE_BUTTON
	&& (modifiers & B_SHIFT_KEY)   == 0
	&& (modifiers & B_OPTION_KEY)  == 0
	&& (modifiers & B_COMMAND_KEY) == 0
	&& (modifiers & B_CONTROL_KEY) == 0
	&&  servers.empty() == false)
	{
		NotifyPopUpMenu *menu;

		menu = new NotifyPopUpMenu (
			servers,
			ConvertToScreen (Bounds()),
			Window()->Frame());
		menu->SetTargetForItems (this);

		menu->Go (
			menu->ScreenLocation(),
			true,
			true,
			ConvertToScreen (Bounds()),
			false);
		delete menu;
	}

	// Shift mouse will take them to
	// the first with nicks then news
	// Nicks takes prececence!!!
	if (nicks || others || news
	&&  buttons == B_PRIMARY_MOUSE_BUTTON
	&& (modifiers & B_SHIFT_KEY)   != 0
	&& (modifiers & B_OPTION_KEY)  == 0
	&& (modifiers & B_COMMAND_KEY) == 0
	&& (modifiers & B_CONTROL_KEY) == 0)
	{
		map<int32, NotifyData *>::const_iterator sit;
		BMessenger msgr;

		for (sit = servers.begin(); 
			!msgr.IsValid() && sit != servers.end() ;
			++sit)
		{
			map<BString, NotifyData *> &clients (sit->second->clients);
			map<BString, NotifyData *>::const_iterator it;

			if (sit->second->nick
			|| (!nicks && sit->second->other)
			|| (!nicks && !others && sit->second->news))
				msgr = sit->second->msgr;

			for (it = clients.begin(); 
				!msgr.IsValid() && it != clients.end();
				++it)
			{
				if (it->second->nick
				|| (!nicks && it->second->other)
				|| (!nicks && !others && it->second->news))
					msgr = it->second->msgr;
			}
		}

		if (msgr.IsValid())
			msgr.SendMessage (M_NOTIFY_SELECT);
	}
}

NotifyPopUpMenu::NotifyPopUpMenu (
	const map<int32, NotifyData *> &servers_,
	BRect vFrame_,
	BRect wFrame_)

	: BPopUpMenu ("NotifyMenu", false, false),

	  servers (servers_),
	  vFrame (vFrame_),
	  wFrame (wFrame_)
{
	map<int32, NotifyData *>::const_iterator sit;

	for (sit = servers.begin(); sit != servers.end(); ++sit)
	{
		if (sit != servers.begin())
			AddSeparatorItem();

		BMessage *msg = new BMessage (M_NOTIFY_SELECT);

		msg->AddBool ("server", true);
		msg->AddMessenger ("msgr", sit->second->msgr);

		AddItem (new NotifyMenuItem (
			sit->second->name.String(),
			msg,
			sit->second->nick,
			sit->second->news,
			sit->second->other));

		map<BString, NotifyData *>::const_iterator it;

		for (it = sit->second->clients.begin();
			it != sit->second->clients.end();
			++it)
		{
			msg = new BMessage (M_NOTIFY_SELECT);

			msg->AddBool ("server", false);
			msg->AddMessenger ("msgr", it->second->msgr);

			AddItem (new NotifyMenuItem (
				it->second->name.String(),
				msg,
				it->second->nick,
				it->second->news,
				it->second->other));
		}
	}

	SetFont (be_plain_font);
}

NotifyPopUpMenu::~NotifyPopUpMenu (void)
{
}

BPoint
NotifyPopUpMenu::ScreenLocation (void)
{
	BPoint point;

	// Make an attempt to place in a pleasing spot
	if (wFrame.top < wFrame.Height())
		point.y = vFrame.bottom;
	else
		point.y = vFrame.top - Bounds().Height();

	if (wFrame.left < wFrame.Width())
		point.x = vFrame.right;
	else
		point.x = vFrame.left - Bounds().Width();

	return point;
}

NotifyMenuItem::NotifyMenuItem (
	const char *name,
	BMessage *msg,
	bool nick_,
	bool news_,
	bool other_)

	: BMenuItem (name, msg),
	  nick (nick_),
	  news (news_),
	  other (other_)
{
}

NotifyMenuItem::~NotifyMenuItem (void)
{
}

void
NotifyMenuItem::DrawContent (void)
{
	
	if (news || nick || other)
	{
		rgb_color was (Menu()->HighColor());
		BPoint point;
		font_height fh;

		point = ContentLocation();
		Menu()->GetFontHeight (&fh);

		Menu()->MovePenTo (
			point.x,
			point.y + fh.ascent + fh.leading);

		rgb_color color;
		
		if (nick)
			color = NickColor;
		else if (other)
			color = OtherColor;
		else
			color = NewsColor;

		Menu()->SetHighColor (color);
		Menu()->DrawString (Label());
		Menu()->SetHighColor (was);
	}
	else BMenuItem::DrawContent();
}
