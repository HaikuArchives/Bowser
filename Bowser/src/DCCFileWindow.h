
#ifndef DCCFILEWINDOW_H_
#define DCCFILEWINDOW_H_

#include <Window.h>
#include <String.h>

class BStatusBar;
class ServerWindow;


class DCCFileWindow : public BWindow
{
	public:
		DCCFileWindow(BString theType, BString theNick, BString theFile,
			BString theSize, BString theIP, BString thePort, ServerWindow *theCaller);
		~DCCFileWindow();
		virtual void Quit();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);
		
		int32 DCCReceive();
		int32 DCCSend();
		
		static int32 dcc_receive(void *arg);
		static int32 dcc_send(void *arg);
		
		void UpdateWindow (int, int, long, bool);
	
	private:
		int mySocket;
		int acceptSocket;
		thread_id dataThread;
		BStatusBar *myStatus;
		BString dNick, dIP, dPort, dSize, dFile, dType;
		long realSize;
		ServerWindow *caller;

		rgb_color dccColor;

};

#endif
