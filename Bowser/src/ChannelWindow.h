
#ifndef CHANNELWINDOW_H_
#define CHANNELWINDOW_H_

#include <String.h>

#include "ClientWindow.h"

class BScrollView;
class ServerWindow;
class NamesView;

class ChannelWindow : public ClientWindow
{
	public:

							ChannelWindow (
								const char *,
								int32,
								const char *,
								const BMessenger &,
								const char *);

							~ChannelWindow (void);

	virtual bool		QuitRequested (void);
	virtual void		MessageReceived (BMessage *);
	virtual void		Parser (const char *);

	virtual void		StateChange (BMessage *msg);

	bool					RemoveUser (const char *);
	int					FindPosition (const char *);
	void					UpdateMode (char, char);
	void					ModeEvent (BMessage *);

	static int			SortNames (const void *, const void *);
	virtual void		TabExpansion (void);

	private:

	BString				chanMode,
							chanLimit,
							chanLimitOld,
							chanKey,
							chanKeyOld,
							lastExpansion;
							
	rgb_color					ctcpReqColor,
									ctcpRpyColor,
									whoisColor,
									errorColor,
									quitColor,
									joinColor,
									noticeColor;

	int32					userCount,
							opsCount;
	friend class		ClientWindow;
	NamesView			*namesList;
	BScrollView			*namesScroll;
};

#endif
