
#ifndef PREFEVENT_H_
#define PREFEVENT_H_

#include <View.h>

class VTextControl;

class PreferenceEvent : public BView
{
	VTextControl				**events;

	public:

									PreferenceEvent (void);
	virtual						~PreferenceEvent (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

const uint32 M_EVENT_MODIFIED						= 'pemo';

#endif
