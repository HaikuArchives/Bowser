
#ifndef SERVERWINDOW_H_
#define SERVERWINDOW_H_

#include <String.h>
#include <Locker.h>
#include <List.h>
#include <Messenger.h>

#include "ClientWindow.h"

class BNetEndpoint;
class BFilePanel;
class ListWindow;
class IgnoreWindow;
class NotifyWindow;

class ServerWindow : public ClientWindow
{
	public:

									ServerWindow (
										const char *,

										BList *,
										const char *,
										const char *,
										const char *,
										const char **,
										bool,
										bool,
										const char *);

	virtual						~ServerWindow (void);
	virtual bool				QuitRequested (void);
	virtual void				MessageReceived (BMessage *message);
	virtual void				MenusBeginning (void);

	static int32				Establish (void *);
	uint32						LocalAddress (void) const;
	void							PostActive (BMessage *);
	void							Broadcast (BMessage *);
	void							RepliedBroadcast (BMessage *);

	protected:

	virtual void				StateChange (BMessage *msg);

	private:

	ClientWindow				*Client (const char *);
	ClientWindow				*ActiveClient (void);

	void							DisplayAll (const char *, bool = false);


	void							SendData (const char *);

	void							ParseLine (const char *);
	BString						FilterCrap (const char *);
		
									// Seperate files:
	bool							ParseEvents (const char *);

	bool							ParseENums (const char *, const char *);

	void							ParseCTCP(BString theNick, BString theMsg);
	void							ParseCTCPResponse(BString theNick, BString theMsg);
	void							DCCGetDialog(BString theNick, BString theFile, BString theSize,
										BString theIP, BString thePort);
	void							DCCChatDialog(BString theNick, BString theIP, BString thePort);


	// Locked accessors:

	static int32				ServerSeed;

	BList							*lnicks;				// Passed from setup
	const BString				lport,		
									lname,
									lident;

	int32							nickAttempt;		// going through list
	BString						myNick;
	BString						quitMsg;

	bool							isConnected,		// were done connecting
									isConnecting,		// in process
									hasWarned,			// warn about quitting
									isQuitting;			// look out, going down

	BNetEndpoint				*endPoint;			// socket class
	static BLocker				identLock;

	char							*send_buffer;		// dynamic buffer for sending
	size_t						send_size;			// size of buffer

	char							*parse_buffer;		// dynamic buffer for parsing
	size_t						parse_size;			// size of buffer

	thread_id					loginThread;		// thread that receives

	BList							clients;				// windows this server "owns"

	rgb_color					ctcpReqColor,
									ctcpRpyColor,
									whoisColor,
									errorColor,
									quitColor,
									joinColor,
									noticeColor;
	const char					**events;
	uint32						localAddress;

	bool							motd,
									initialMotd,
									identd;
	BString						cmds;

};

const uint32 M_SERVER_ALTW					= 'rwcw'; // serveR Window Close Window

#endif
