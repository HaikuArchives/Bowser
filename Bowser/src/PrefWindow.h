
#ifndef PREFWINDOW_H_
#define PREFWINDOW_H_

#include <View.h>

class BCheckBox;

class PreferenceWindow : public BView
{
	BCheckBox					*messageBox,
									*windowFollows,
									*hideServer,
									*showServer,
									*showTopic;

	public:

									PreferenceWindow (void);
	virtual						~PreferenceWindow (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
