
#ifndef LISTWINDOW_H_
#define LISTWINDOW_H_

#include <Window.h>
//#include <Messenger.h>
//#include <List.h>
#include <String.h>

#include <regex.h>

class BListView;
class BScrollView;
class BMenuItem;
class StatusView;
class WindowSettings;

class ListWindow : public BWindow
{
	BListView				*listView;
	BScrollView				*scroller;
	StatusView				*status;
	BList						list,
								showing;
	BString					filter,
								find;
	regex_t					re, fre;
	bool						processing;
	float						channelWidth,
								topicWidth,
								sChannelWidth,
								sTopicWidth,
								sLineWidth;

	BMenuItem				*mChannelSort,
								*mUserSort,
								*mFilter,
								*mFind,
								*mFindAgain;

	WindowSettings			*settings;

	public:

								ListWindow (
									BRect,
									const char *);
	virtual					~ListWindow (void);
	virtual bool			QuitRequested (void);
	virtual void			MessageReceived (BMessage *);
	virtual void			FrameResized (float, float);

	static int				SortChannels (const void *, const void *);
	static int				SortUsers (const void *, const void *);

	float						ChannelWidth (void) const;
};

#endif
