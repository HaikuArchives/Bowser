
#ifndef STATUSVIEW_H_
#define STATUSVIEW_H_

#define STATUS_ALIGN_RIGHT					0
#define STATUS_ALIGN_LEFT					1

#include <View.h>
#include <List.h>
#include <String.h>

class StatusItem;

class StatusView : public BView
{
	BList					items;

	public:

							StatusView (BRect);
	virtual				~StatusView (void);

	void					AddItem (StatusItem *, bool);
	StatusItem			*ItemAt (int32) const;

	void					SetItemValue (int32, const char *);
	virtual void		Draw (BRect);

	protected:

	void					DrawSplit (float);
};

class StatusItem
{
	BString				label,
							value;
	BRect					frame;
	int32					alignment;

	friend				StatusView;


	public:

							StatusItem (
								const char *,
								const char *,
								int32 = STATUS_ALIGN_RIGHT);

	virtual				~StatusItem (void);
};

#endif
