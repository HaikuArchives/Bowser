
#ifndef HISTORYMENU_H_
#define HISTORYMENU_H_

#define BACK_BUFFER_SIZE			20

#include <String.h>

class VTextControl;

class HistoryMenu : public BView
{
	BString					backBuffer[BACK_BUFFER_SIZE];
	int32						bufferFree, bufferPos;

	bool						tracking;
	bigtime_t				mousedown;
	rgb_color				tricolor;

	public:

								HistoryMenu (BRect);
	virtual					~HistoryMenu (void);
	virtual void			Draw (BRect);
	virtual void			AttachedToWindow (void);
	virtual void			MouseDown (BPoint);
	virtual void			MouseMoved (BPoint, uint32, const BMessage *);
	virtual void			MouseUp (BPoint);

	void						PreviousBuffer (VTextControl *);
	void						NextBuffer (VTextControl *);
	BString					Submit (const char *);
	bool						HasHistory (void) const;
	void						DoPopUp (bool);
};

#endif
