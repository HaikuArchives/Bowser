
#ifndef MESSAGEWINDOW_H_
#define MESSAGEWINDOW_H_

#include <TextControl.h>
#include <String.h>

#include "ClientWindow.h"

class ServerWindow;

class MessageWindow : public ClientWindow
{
	public:

									MessageWindow (
										const char *,
										int32,
										const char *,
										const BMessenger &,
										const char *,
										const char *,
										bool = false,
										bool = false,
										const char * = "",
										const char * = "");

									~MessageWindow (void);
	virtual bool				QuitRequested (void);
	virtual void				MessageReceived (BMessage *);
	virtual void				Show (void);
	virtual void				Parser (const char *);
	virtual void				DroppedFile (BMessage *);
	virtual void				TabExpansion (void);

	void							UpdateString (void);
	
	void 							DCCServerSetup (void);
	static int32				DCCIn (void *);
	static int32				DCCOut (void *);
	bool							DataWaiting (void);

	private:
	BString						chatAddy,
									chatee;

	bool							dChat,
									dInitiate,
									dConnected;

	int							mySocket,
									acceptSocket;

	thread_id					dataThread;

	BString						dIP,
									dPort;
};

#endif
