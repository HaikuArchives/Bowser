
#ifndef PREFNOTIFY_H_
#define PREFNOTIFY_H_

#include <View.h>

class BMenuField;
class BCheckBox;
class BTextControl;

class PreferenceNotify : public BView
{
	BCheckBox					*flashNickBox,
									*flashContentBox,
									*flashOtherNickBox;
	BMenuField					*notifyMenu;
	BTextControl				*akaEntry,
									*otherNickEntry,
									*autoNickTimeEntry;

	public:

									PreferenceNotify (void);
	virtual						~PreferenceNotify (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
