
#ifndef PREFDCC_H_
#define PREFDCC_H_

#include <View.h>

class BCheckBox;

class PreferenceDCC : public BView
{
	public:

									PreferenceDCC (void);
	virtual						~PreferenceDCC (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
