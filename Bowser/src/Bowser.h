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

#ifndef _BOWSER_H_
#define _BOWSER_H_

#include <Application.h>

#include "DCCFileWindow.h"
#include "IRCDefines.h"

class AppSettings;
class SetupWindow;
class ServerData;
class AboutWindow;
class DCCFileWindow;
extern class BowserApp * bowser_app;

class BowserApp : public BApplication
{
  public:
                            BowserApp (void);
    virtual                 ~BowserApp (void);

    virtual void            MessageReceived (BMessage *);
    virtual void            AboutRequested (void);
    virtual bool            QuitRequested (void);
    virtual void            ArgvReceived (int32, char **);
    virtual void            ReadyToRun (void);

    BString                 BowserVersion (void);

    void                    StampState (bool);
    bool                    GetStampState (void) const;
    void                    ParanoidState (bool);
    bool                    GetParanoidState (void) const;
    void                    WindowFollowsState (bool);
    bool                    GetWindowFollowsState (void) const;
    void                    HideSetupState (bool);
    bool                    GetHideSetupState (void) const;
    void                    ShowSetupState (bool);
    bool                    GetShowSetupState (void) const;
    void                    ShowTopicState (bool);
    bool                    GetShowTopicState (void) const;
    void                    StatusTopicState (bool);
    bool                    GetStatusTopicState (void) const;
    void                    AltwSetupState (bool);
    bool                    GetAltwSetupState (void) const;
    void                    AltwServerState (bool);
    bool                    GetAltwServerState (void) const;
    void                    MasterLogState (bool);
    bool                    GetMasterLogState (void) const;
    void                    DateLogsState (bool);
    bool                    GetDateLogsState (void) const;


    void                    AlsoKnownAs (const char *);
    BString                 GetAlsoKnownAs (void) const;
    void                    OtherNick (const char *);
    BString                 GetOtherNick (void) const;
    void                    AutoNickTime (const char *);
    BString                 GetAutoNickTime (void) const;

    void                    NotificationMask (int32);
    int32                   GetNotificationMask (void) const;
    void                    MessageOpenState (bool);
    bool                    GetMessageOpenState (void) const;
    void                    MessageFocusState (bool);
    bool                    GetMessageFocusState (void);
    void                    NicknameBindState (bool);
    bool                    GetNicknameBindState (void) const;

    BString                 GetNickname (uint32) const;
    uint32                  GetNicknameCount (void) const;
    bool                    AddNickname (ServerData *, const char *);
    bool                    RemoveNickname (ServerData *, uint32);

    void                    ClientFontFamilyAndStyle (int32, const char *,
                              const char *);
    void                    ClientFontSize (int32, float);
    const BFont             *GetClientFont (int32) const;

    rgb_color               GetColor (int32) const;
    void                    SetColor (int32, const rgb_color);

    BString                 GetEvent (int32) const;
    void                    SetEvent (int32, const char *);

    BString                 GetCommand (int32);
    void                    SetCommand (int32, const char *);

    bool                    CanNotify (void) const;
    void                    Broadcast (BMessage *);
    void                    Broadcast (BMessage *, const char *, bool = false);

    ServerData              *FindServerData (const char *);

  private:
    AppSettings             *settings;
    SetupWindow             *setupWindow;
    DCCFileWindow           *dccFileWin;
    AboutWindow             *aboutWin;
    sem_id                  dcc_sid;

};


#endif
