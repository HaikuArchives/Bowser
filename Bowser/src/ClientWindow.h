
#ifndef CLIENTWINDOW_H_
#define CLIENTWINDOW_H_

#define STATUS_SERVER			0
#define STATUS_LAG					1
#define STATUS_NICK				2
#define STATUS_USERS				3
#define STATUS_OPS				4
#define STATUS_MODES				5
#define STATUS_TOPIC			6

#include <Window.h>
#include <Messenger.h>
#include <String.h>
#include <Font.h>
#include <File.h>
//#include <Path.h>

class BMenuBar;
class BMenu;
class BMenuItem;
class BView;
class BStringView;
class BTextControl;
class BScrollView;

class BTextView;
class IRCView;
class ServerWindow;
class WindowSettings;
class ClientInputFilter;
class CommandWrapper;
class CommandWrapperNP;
class StatusView;
class HistoryMenu;

class ClientWindow : public BWindow
{
	protected:

	BString							id;
	const int32						sid;
	const BString					serverName;
	BMessenger						sMsgr;
	BString							myNick;
	BString							alsoKnownAs,
										otherNick,
										autoNickTime;
	int32								notifyMask;
	BList								nickTimes;

	BMenuBar							*menubar;
	BMenu								*mServer,
										*mWindow,
										*mHelp;
	BMenuItem						*mNotification[3],
										*mHistory,
										*mIgnore,
										*mList,
										*mNotifyWindow,
										*mScrolling;
	BView								*bgView;
	StatusView						*status;
	BTextControl					*input;
	HistoryMenu						*history;
	BScrollView						*scroll;
	IRCView							*text;

	rgb_color						textColor,
										ctcpReqColor,
										nickColor,
										quitColor,
										errorColor,
										joinColor,
										whoisColor,
										myNickColor,
										actionColor,
										opColor,
										inputbgColor,
										inputColor;

	BFont								myFont,
										serverFont,
										inputFont;
	bool								timeStampState,
										canNotify,
										scrolling,
										isLogging;

	WindowSettings					*settings;

	static const char				*endl;


	friend							ClientInputFilter;



	virtual void					Display (
											const char *,
											const rgb_color *,
											const BFont * = 0,
											bool = false);

	virtual void					Submit (const char *, bool = true, bool = true);

	virtual void					Parser (const char *);
	virtual bool					SlashParser (const char *);
	virtual void					StateChange (BMessage *);

	void								AddSend (BMessage *, const char *);
	void								AddSend (BMessage *, const BString &);
	void								AddSend (BMessage *, int32);

	void								SetupLogging (void);
	BFile								logFile;

	static int32					TimedSubmit (void *);
	void								PackDisplay (BMessage *,
											const char *,
											const rgb_color * = 0,
											const BFont * = 0,
											bool = false);
	int32								FirstKnownAs (
											const BString &,
											BString &,
											bool *);
	int32								FirstSingleKnownAs (
											const BString &,
											const BString &);


	public:
	
	typedef void (ClientWindow::*CmdFunc) (const char *);

										ClientWindow (
											const char *,
											int32,
											const char *,
											const BMessenger &,
											const char *,
											BRect,
											WindowSettings * = 0);

										ClientWindow (
											const char *,
											int32,
											const char *,
											const char *,
											BRect,
											WindowSettings * = 0);

	virtual							~ClientWindow (void);

	virtual bool					QuitRequested (void);
	virtual void					DispatchMessage (BMessage *, BHandler *);
	virtual void					MessageReceived (BMessage *);
	virtual void					WindowActivated (bool);
	virtual void					Show (void);
	virtual void					MenusBeginning (void);
	virtual void					MenusEnded (void);
	virtual void					FrameResized (float, float);

	const BString					&Id (void) const;
	virtual void					TabExpansion (void);
	virtual void					DroppedFile (BMessage *);
	static int32					DNSLookup (void *);
	static int32					ExecPipe (void *);

	bool								Scrolling (void) const;
	void								ChannelMessage (
											const char *,
											const char * = 0,
											const char * = 0,
											const char * = 0);
	void								ActionMessage (
											const char *,
											const char *);
	float								ScrollPos(void);
	void								SetScrollPos(float);
	void								ScrollRange(float *, float *);

	protected:
										// Commands
	// gone!
	
	BString								DurationString(int64);
	
	void								CTCPAction (BString theTarget, BString
											theMsg);

	private:
	
	void								Init (void);
	void								NotifyRegister (void);
	
	bool								ParseCmd (const char *);
};

class TimedNick
{
	public:

	BString							nick;
	bigtime_t						when;

										TimedNick (
											const char *nick_,
											bigtime_t when_)
										: nick (nick_),
										  when (when_)
										{ }
};

const uint32 M_BOWSER_BUGS				= 'cwbb';

#endif
