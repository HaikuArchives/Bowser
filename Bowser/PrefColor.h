
#ifndef PREFCOLOR_H_
#define PREFCOLOR_H_

#include <View.h>
#include <Messenger.h>

class ColorLabel;

class PreferenceColor : public BView
{
	ColorLabel					**colors;
	BMessenger					picker;

	int32							last_invoke;
	bool							had_picker;
	BPoint						pick_point;

	public:

									PreferenceColor (void);
	virtual						~PreferenceColor (void);
	virtual void				AttachedToWindow (void);
	virtual void				DetachedFromWindow (void);
	virtual void				MessageReceived (BMessage *);
};

#endif
