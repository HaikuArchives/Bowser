
#ifndef IRCVIEW_H_
#define IRCVIEW_H_

#include <TextView.h>

class BTextControl;
class BFont;

struct IRCViewSettings;

class IRCView : public BTextView
{
	bool					tracking;
	float					lasty;
	public:
								IRCView (
									BRect,
									BRect,
									BTextControl *);

								~IRCView (void);
	virtual void			MouseDown (BPoint);
	virtual void			KeyDown(const char * bytes, int32 numBytes);
	virtual void			FrameResized (float, float);

	int32						DisplayChunk (
									const char *,
									const rgb_color *,
									const BFont *);

	int32						URLLength (const char *);
	int32						FirstMarker (const char *);
	void						ClearView (bool);

	void						SetColor (int32, rgb_color);
	void						SetFont  (int32, const BFont *);
	
	private:

	IRCViewSettings		*settings;

};


#endif
