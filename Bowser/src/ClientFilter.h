
#ifndef CLIENTINPUTFILTER_H_
#define CLIENTINPUTFILTER_H_

#include <MessageFilter.h>

class ClientWindow;


class ClientInputFilter : public BMessageFilter
{
	ClientWindow					*window;
	bool								handledDrop;

	public:
										ClientInputFilter (ClientWindow *);
	virtual							~ClientInputFilter (void);
	virtual filter_result			Filter (BMessage *, BHandler **);

	filter_result					HandleKeys (BMessage *);
	void							HandleDrop (const char *);
};

#endif
