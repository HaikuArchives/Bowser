
#ifndef RUNVIEW_H_
#define RUNVIEW_H_

#include <View.h>
#include <Font.h>

#include <list>

#include "IRCDefines.h"

class BScrollView;
class RunBuffer;
class RunView;

class RunSelection
{
	public:

	RunBuffer			*runner;
	int32					offset;

							RunSelection (void);
							~RunSelection (void);
};

class RunView : public BView
{
	RunBuffer			*firstBuffer,
							*lastBuffer;
	float					spacer;

	BFont					fonts[MAX_FONTS];
	rgb_color			colors[MAX_COLORS];
	BScrollView			*scroller;
	RunSelection		selection[2];

	RunSelection		trackSelection;
	bool					tracking;


	friend				RunBuffer;


	void					UpdateScrollBar (void);
	BRect					SelectionFrame (void);
	void					DrawPortion (RunBuffer *, int32, float, float, int32, int32, int32);
	void					DrawSelection (RunBuffer *, int32, float, float, int32, int32, int32);

	public:

							RunView (BRect);
	virtual				~RunView (void);

	virtual void		FrameResized (float, float);
	virtual void		Draw (BRect);

	virtual void		TargetedByScrollView (BScrollView *);
	virtual void		AttachedToWindow (void);
	virtual void		MouseDown (BPoint);
	virtual void		MouseMoved (BPoint, uint32, const BMessage *);
	virtual void		MouseUp (BPoint);
	virtual void		KeyDown (const char *, int32);
	virtual void		MakeFocus (bool = true);
	virtual void		WindowActivated (bool);
	virtual void		MessageReceived (BMessage *msg);

	void					Append (
								const char *,
								int32 = 0,
								int32 = 0,
								bool = false);
	void					DeselectAll (void);

	rgb_color			GetIndexedColor (int32) const;
	const BFont			*GetIndexedFont (int32) const;
};

#endif
