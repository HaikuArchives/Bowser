/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * Bowser's source code is provided without any warranty what-so-ever; users may
 * use the source code at their own risk. In no way may the developers who have
 * contributed to the code be held accountable for damages that arise from the use
 * or misuse of the source code.
 * 
 * Anyone is free to use the source code however they like as long as credit is
 * given to The Bowser Team (http://bowser.sourceforge.net) in both the
 * documentation for the program and the source code (if you choose to release it.)
 * Any derivitive work using this code must _not_ use the name Bowser or some sort
 * of variation thereof unless explicit permission is granted from The Bowser Team
 * in advance. In addition, commercial developers may not use this source code
 * without explicit permssion from The Bowser Team in advance.
 *
 */

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
                                
    virtual bool                QuitRequested (void);

    virtual void                MessageReceived (BMessage *);
    virtual void                Show (void);
    virtual void                Parser (const char *);
    virtual void                DroppedFile (BMessage *);
    virtual void                TabExpansion (void);
    virtual void                StateChange (BMessage *);

    void                        DCCServerSetup (void);

    static int32                DCCIn (void *);
    static int32                DCCOut (void *);

    bool                        DataWaiting (void);

  private:
    BString                     chatAddy,
                                chatee,
                                dIP,
                                dPort;

    bool                        dChat,
                                dInitiate,
                                dConnected;

    int                         mySocket,
                                acceptSocket;

    thread_id                   dataThread;
};

#endif
