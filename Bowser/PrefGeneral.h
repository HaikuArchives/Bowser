
#ifndef PREFGENERAL_H_
#define PREFGENERAL_H_

#include <View.h>

class BCheckBox;

class PreferenceGeneral : public BView
{
	BCheckBox					*stampBox,
									*paranoidBox,
									*messageBox,
									*messageFocus,
									*nickBindBox,
									*windowFollows,
									*hideServer,
									*showServer;

	public:

									PreferenceGeneral (void);
	virtual						~PreferenceGeneral (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
