
#ifndef NOTIFYWINDOW_H_
#define NOTIFYWINDOW_H_

#include <Window.h>
#include <Messenger.h>

class BListView;
class BMessageRunner;
class BList;

class NotifyWindow : public BWindow
{
	BListView				*listView;
	StatusView				*status;
	WindowSettings			*settings;

	BMenuItem				*mAdd,
								*mRemove,
								*mClear;

	public:

								NotifyWindow (
									BList *,
									BRect,
									const char *);
	virtual					~NotifyWindow (void);
	virtual bool			QuitRequested (void);
	virtual void			MessageReceived (BMessage *);
	virtual void			MenusBeginning (void);
};

class NotifyItem : public BStringItem
{
	bool							on;

	public:

									NotifyItem (const char *, bool);
									NotifyItem (const NotifyItem &);
	virtual						~NotifyItem (void);
	virtual void				DrawItem (BView *, BRect, bool = false);

	bool							On (void) const;
	void							SetOn (bool);
};

#endif
