
#ifndef PREFWINDOW_H_
#define PREFWINDOW_H_

#include <View.h>

class BCheckBox;

class PreferenceWindow : public BView
{
	BCheckBox					*messageBox,
									*windowFollows,
									*hideSetup,
									*showSetup,
									*showTopic,
									*AltWSetup,
									*AltWServer;

	public:

									PreferenceWindow (void);
	virtual						~PreferenceWindow (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

const uint32 M_MESSAGE_OPEN     					= 'pwmo';
const uint32 M_MESSAGE_FOCUS						= 'pwmf';
const uint32 M_HIDE_SETUP							= 'pwhs';
const uint32 M_ACTIVATE_SETUP						= 'pwas';
const uint32 M_SHOW_TOPIC							= 'pwst';
const uint32 M_ALTW_SETUP							= 'pwap';
const uint32 M_ALTW_SERVER							= 'pwar';

#endif
