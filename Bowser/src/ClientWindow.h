
#ifndef CLIENTWINDOW_H_
#define CLIENTWINDOW_H_

#include <Window.h>
#include <Messenger.h>
#include <String.h>
#include <Font.h>

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
										myNickColor,
										actionColor,
										opColor;

	BFont								myFont,
										serverFont,
										inputFont;
	bool								timeStampState,
										canNotify,
										scrolling;

	WindowSettings					*settings;

	static const char				*endl;

	CommandWrapper					*cmdWrap;


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
	void								PingCmd (const char *);
	void								VersionCmd (const char *);
	void								NoticeCmd (const char *);
	void								RawCmd (const char *);
	void								StatsCmd (const char *);
	void								QuitCmd (const char *);
	void								KillCmd (const char *);
	void								WallopsCmd (const char *);
	void								TraceCmd (const char *);
	void								MeCmd (const char *);
	void								DescribeCmd (const char *);
	void								AwayCmd (const char *);
	void								BackCmd (const char *);
	void								JoinCmd (const char *);
	void								NickCmd (const char *);
	void								MsgCmd (const char *);
	void								NickServCmd (const char *);
	void								ChanServCmd (const char *);
	void								MemoServCmd (const char *);
	void								UserhostCmd (const char *);
	void								CtcpCmd (const char *);
	void								AdminCmd (const char *);
	void								InfoCmd (const char *);
	void								OperCmd (const char *);
	void								RehashCmd (const char *);
	void								KickCmd (const char *);
	void								WhoIsCmd (const char *);
	void								PartCmd (const char *);
	void								OpCmd (const char *);
	void								DopCmd (const char *);
	void								ModeCmd (const char *);
	void								Mode2Cmd (const char *);
	void								MotdCmd (const char *);
	void								TopicCmd (const char *);
	void								NamesCmd (const char *);
	void								QueryCmd (const char *);
	void								WhoCmd (const char *);
	void								WhoWasCmd (const char *);
	void								DccCmd (const char *);
	void								ClearCmd (const char *);
	void								InviteCmd (const char *);
	void								ListCmd (const char *);
	void								IgnoreCmd (const char *);
	void								UnignoreCmd (const char *);
	void								ExcludeCmd (const char *);
	void								NotifyCmd (const char *);
	void								UnnotifyCmd (const char *);
	void								PreferencesCmd (const char *);
	void								AboutCmd (const char *);
	void								VisitCmd (const char *);
	void								SleepCmd (const char *);
	
	void								CTCPAction (BString theTarget, BString
											theMsg);

	private:

	void								Init (void);
	void								NotifyRegister (void);
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


#endif
