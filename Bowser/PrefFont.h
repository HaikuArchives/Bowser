
#ifndef PREFFONT_H_
#define PREFFONT_H_

#include <View.h>

class BMenuField;

class PreferenceFont : public BView
{
	BMenuField					*clientFont[5],
									*fontSize[5];

	public:

									PreferenceFont (void);
	virtual						~PreferenceFont (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
