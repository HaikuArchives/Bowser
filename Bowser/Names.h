
#ifndef NAMES_H_
#define NAMES_H_

#include <ListItem.h>
#include <ListView.h>
#include <String.h>

class BPopUpMenu;

class NameItem : public BListItem
{
	public:

										NameItem (
											const char *,
											int32);
	virtual							~NameItem();
	BString							Name (void) const;
	BString							Address (void) const;
	int32								Status (void) const;

	void								SetName (const char *);
	void								SetAddress (const char *);
	void								SetStatus (int32);

	virtual void					DrawItem (BView *father, BRect frame, bool complete = false);

	private:

	BString							myName,
										myAddress;
	int32								myStatus;
};

class NamesView : public BListView
{
	public:

										NamesView (BRect frame);
	virtual							~NamesView();
	virtual void					AttachedToWindow();
	virtual void					MouseDown(BPoint myPoint);

	void								SetColor (int32, rgb_color);
	rgb_color						GetColor (int32) const;
	void								SetFont (int32, const BFont *);
	
	private:

	BPopUpMenu						*myPopUp;
	BMenu 							*CTCPPopUp;
	int32								lastSelected;
	int32								lastButton;

	rgb_color						opColor,
										voiceColor,
										textColor,
										ignoreColor,
										bgColor;
};


#endif
