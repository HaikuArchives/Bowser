
#ifndef SERVEROPTIONS_H_
#define SERVEROPTIONS_H_

#include <Window.h>
#include <Messenger.h>

class ServerData;
class BCheckBox;
class BTextView;
class BScrollView;
class SetupWindow;
class WindowSettings;

class ServerOptions : public BWindow
{
	ServerData		*serverData;
	BMessenger		suMsgr;

	BCheckBox		*connectBox,
					*motdBox,
					*identdBox;
	BTextView		*connectText;
	BScrollView		*scroller;
	WindowSettings	*settings;

	float				width;

	friend			SetupWindow;

	public:

						ServerOptions (
							BRect,
							ServerData *,
							const BMessenger &);
	virtual			~ServerOptions (void);
	virtual bool	QuitRequested (void);
	virtual void	MessageReceived (BMessage *);
	virtual void	Hide (void);
	virtual void	Show (void);
};

#endif
