
#ifndef SETUPWINDOW_H_
#define SETUPWINDOW_H_

#include <Window.h>
#include <String.h>
#include <List.h>

#include <vector>

class BMessageRunner;
class SetupView;
class ServerOptions;
class PrefsFrame;
class IgnoreWindow;
class NotifyWindow;
class ListWindow;

class ServerData
{
	public:

	BString						server,
									port,
									name,
									ident,
									connectCmds;
	bool							motd,
									identd,
									connect;
									// Order stores *indexes*
									// into of the the application's
									// nickname list.  One single list
									// of nicknames.  Users specify the
									// order for each server.
	vector<uint32>				order;
	BList							ignores,
									notifies;
	ServerOptions				*options;

	IgnoreWindow				*ignoreWindow;
	NotifyWindow				*notifyWindow;
	ListWindow					*listWindow;
	int32							instances,
									connected;

	BMessageRunner				*notifyRunner;
	BString						lastNotify;

									ServerData (void);
									~ServerData (void);

	void							Copy (const ServerData &);
	BListItem					*AddNotifyNick (const char *);
	BListItem					*RemoveNotifyNick (const char *);
};

class SetupWindow : public BWindow
{
	public:
									SetupWindow (BRect frame, BList *, int32, const BString *);
	virtual						~SetupWindow (void);
	virtual bool				QuitRequested (void);
	virtual void				MessageReceived (BMessage *message);

	private:
	
	void							Connect (int32);
	void							SetCurrent (void);
	bool							Validate (void);
	bool							SaveMarked (void);

	SetupView					*bgView;
	int32							quitCount;
	PrefsFrame					*prefsFrame;
};

#endif