
#ifndef PREFLOG_H_
#define PREFLOG_H_

#include <View.h>

class BCheckBox;

class PreferenceLog : public BView
{
	BCheckBox					*masterLog,
								*dateLogs;

	public:

									PreferenceLog (void);
	virtual						~PreferenceLog (void);
	virtual void				AttachedToWindow (void);
	virtual void				MessageReceived (BMessage *);
};

const uint32 M_MASTER_LOG     					= 'plml';
const uint32 M_DATE_LOGS     					= 'pldl';


#endif
