
#ifndef PREFCOMMAND_H_
#define PREFCOMMAND_H_

#include <View.h>

class VTextControl;

class PreferenceCommand : public BView
{
	VTextControl				**commands;

	public:

									PreferenceCommand (void);
	virtual						~PreferenceCommand (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

const uint32 M_COMMAND_MODIFIED						= 'pcmo';

#endif
