
#ifndef PREFGENERAL_H_
#define PREFGENERAL_H_

#include <View.h>

class BCheckBox;

class PreferenceGeneral : public BView
{
	BCheckBox					*stampBox,
									*paranoidBox,
									*nickBindBox,
									*autoRejoin,
									*altwSetup,
									*altwServer;

	public:

									PreferenceGeneral (void);
	virtual						~PreferenceGeneral (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

const uint32 M_STAMP_BOX							= 'pgsb';
const uint32 M_STAMP_PARANOID						= 'pgpa';
const uint32 M_NICKNAME_BIND						= 'pgnb';
const uint32 M_AUTO_REJOIN							= 'pgar';

#endif
