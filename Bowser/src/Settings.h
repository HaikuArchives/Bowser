
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <Message.h>
#include <String.h>
#include <Path.h>
#include <Rect.h>

class BWindow;

class Settings
{
	BMessage						packed;
	BString						label;
	BPath							path;
	bool							restored;

	public:

									Settings (
										const char *,
										const char *);

	virtual						~Settings (void);

	void							Save (void);
	void							Restore (void);
	bool							Restored (void) const;

	protected:
	virtual void				SavePacked (BMessage *);
	virtual void				RestorePacked (BMessage *);
};

class WindowSettings : public Settings
{
	BWindow						*window;

	public:

	bool							notification[3];
	

									WindowSettings (	
										BWindow *,
										const char *,
										const char *);

	virtual						~WindowSettings (void);
	BRect							Frame (void) const;

	protected:

	virtual void				SavePacked (BMessage *);
	virtual void				RestorePacked (BMessage *);
};

#endif


