
#ifndef PREFSFRAME_H_
#define PREFSFRAME_H_

#include <Window.h>

class BListView;
class BScrollView;
class BBox;
class BView;
class PrefsItem;
class WindowSettings;

class PrefsFrame : public BWindow
{
	BBox						*container;
	BListView				*list;
	BScrollView				*scroller;
	PrefsItem				*current;

	BWindow					*setupWindow;
	WindowSettings			*settings;


	public:

								PrefsFrame (BWindow *);
	virtual					~PrefsFrame (void);

	virtual bool			QuitRequested (void);
	virtual void			MessageReceived (BMessage *msg);
	virtual void			Show (void);

	void						AddPreference (
									const char *,
									const char *,
									BView *,
									bool = true);
};

#endif
