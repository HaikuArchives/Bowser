#ifndef IGNOREWINDOW_H_
#define IGNOREWINDOW_H_

#include <Window.h>
#include <Messenger.h>
#include <ListItem.h>
#include <List.h>

#include <regex.h>

#include "Prompt.h"

class BList;
class BOutlineListView;
class BMenuItem;
class StatusView;
class WindowSettings;

class IgnoreItem : public BListItem
{
	char						*text;
	BList						excludes;
	regex_t					re;

	public:

								IgnoreItem (const char *);
								IgnoreItem (const IgnoreItem &);
	virtual					~IgnoreItem (void);
	bool						IsAddress (void) const;
	bool						IsMatch (const char *, const char * = 0) const;
	const char				*Text (void) const;
	void						SetText (const char *);

	bool						AddExclude (const char *);
	bool						AddExclude (BStringItem *);
	bool						RemoveExclude (const char *);

	int32						CountExcludes (void) const;
	BStringItem				*ExcludeAt (int32) const;

	virtual void			DrawItem (BView *, BRect, bool = false);
};

class IgnoreWindow : public BWindow
{
	BOutlineListView		*listView;
	StatusView				*status;
	WindowSettings			*settings;

	BMenuItem				*mAdd,
								*mExclude,
								*mRemove,
								*mClear;

	public:

								IgnoreWindow (
									BList *,
									BRect,
									const char *);
	virtual					~IgnoreWindow (void);
	virtual bool			QuitRequested (void);
	virtual void			MessageReceived (BMessage *);
	virtual void			MenusBeginning (void);

	private:

	void						UpdateCount (void);
};

#endif
