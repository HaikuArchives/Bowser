// Bowser.h : part of the Bowser IRC client, rebirthed on 4/24/99

#ifndef _BOWSER_H_
#define _BOWSER_H_

#include <Application.h>
#include <String.h>

#include "IRCDefines.h"

class AppSettings;
class SetupWindow;
class ServerData;

extern class BowserApp * bowser_app;

class BowserApp : public BApplication
{
	public:

								BowserApp (void);
	virtual					~BowserApp (void);

	virtual void			MessageReceived (BMessage *);
	virtual void			AboutRequested (void);
	virtual bool			QuitRequested (void);
	virtual void			ArgvReceived (int32, char **);
	virtual void			ReadyToRun (void);
			
	void						StampState (bool);
	bool						GetStampState (void) const;
	void						ParanoidState (bool);
	bool						GetParanoidState (void) const;
	void						WindowFollowsState (bool);
	bool						GetWindowFollowsState (void) const;
	void						HideServerState (bool);
	bool						GetHideServerState (void) const;
	void						ShowServerState (bool);
	bool						GetShowServerState (void) const;
	void						ShowTopicState (bool);
	bool						GetShowTopicState (void) const;
	void						AutoRejoinState (bool);
	bool						GetAutoRejoinState (void) const;

	void						AlsoKnownAs (const char *);
	BString					GetAlsoKnownAs (void) const;
	void						OtherNick (const char *);
	BString					GetOtherNick (void) const;
	void						AutoNickTime (const char *);
	BString					GetAutoNickTime (void) const;

	void						NotificationMask (int32);
	int32						GetNotificationMask (void) const;
	void						MessageOpenState (bool);
	bool						GetMessageOpenState (void) const;
	void						MessageFocusState (bool);
	bool						GetMessageFocusState (void);
	void						NicknameBindState (bool);
	bool						GetNicknameBindState (void) const;

	BString					GetNickname (uint32) const;
	uint32					GetNicknameCount (void) const;
	bool						AddNickname (ServerData *, const char *);
	bool						RemoveNickname (ServerData *, uint32);

	void						ClientFontFamilyAndStyle (int32, const char *,
									const char *);
	void						ClientFontSize (int32, float);
	const BFont				*GetClientFont (int32) const;

	rgb_color				GetColor (int32) const;
	void						SetColor (int32, const rgb_color);

	BString					GetEvent (int32) const;
	void						SetEvent (int32, const char *);

	BString					GetCommand (int32);
	void						SetCommand (int32, const char *);

	bool						CanNotify (void) const;
	void						Broadcast (BMessage *);
	void						Broadcast (BMessage *, const char *, bool = false);

	ServerData 				*FindServerData (const char *);

	private:

	AppSettings				*settings;
	SetupWindow				*setupWindow;
};


#endif
