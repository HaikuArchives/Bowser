
#ifndef IRCSTATE_H_
#define IRCSTATE_H_

#include <GraphicsDefs.h>

class RunView;

enum ColorGround
{
	ColorForeground,
	ColorBackground
};


class StateInterface
{
	int32					offset;

	public:

							StateInterface (int32);
	virtual				~StateInterface (void);

	int32					Offset (void) const;
	virtual void		Apply (RunView *) const = 0;
	virtual float		GetFontHeight (RunView *) const;
};

class ColorIndexState : public StateInterface
{
	int32					index;
	ColorGround			ground;

	public:

							ColorIndexState (
								int32,
								int32,
								ColorGround = ColorForeground);

	virtual				~ColorIndexState (void);
	virtual void		Apply (RunView *) const;
};

class ColorStaticState : public StateInterface
{
	rgb_color			color;
	ColorGround			ground;

	public:

							ColorStaticState (
								int32,
								rgb_color,
								ColorGround = ColorForeground);

	virtual				~ColorStaticState (void);
	virtual void		Apply (RunView *) const;
};

class FontState : public StateInterface
{
	int32					index;

	public:
			
							FontState (int32, int32);
	virtual				~FontState (void);
	virtual void		Apply (RunView *) const;
	virtual float		GetFontHeight (RunView *) const;
};

#endif
