
#ifndef NOTIFYVIEW_H_
#define NOTIFYVIEW_H_

#include <View.h>
#include <String.h>
#include <Entry.h>

#include <map>

class BBitmap;
class BMenuItem;
class BMessageRunner;
struct NotifyData;

class _EXPORT NotifyView : public BView
{
	bool									newsOn, nickOn, otherOn;
	BBitmap								*bitmap[4];
	entry_ref							app_ref;
	BString								signature;

											// Left in header, cuz only
											// two files include this one.
											// not too bad
	map<int32, NotifyData *>		servers;
	int32									news,
											nicks,
											others,
											mask;

	// We don't rely on Pulse because a lot of system monitors that
	// place themselves in the Deskbar use that. Got our own stinking
	// heartbeat
	BMessageRunner						*runner;


	public:

											NotifyView (int32);
											NotifyView (BMessage *);
	virtual								~NotifyView (void);

	virtual status_t					Archive (BMessage *, bool = true) const;
	static NotifyView					*Instantiate (BMessage *);

	virtual void						Draw (BRect);
	virtual void						MessageReceived (BMessage *);
	virtual void						AttachedToWindow (void);
	virtual void						DetachedFromWindow (void);
	virtual void						MouseDown (BPoint);
};


#endif
