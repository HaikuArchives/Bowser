
#ifndef IRCDEFINES_H_
#define IRCDEFINES_H_

#define	DEV_BUILD

#define BOWSER_SETTINGS									"Bowser_Settings"

#define NOTIFY_NICK_BIT									0x0001
#define NOTIFY_CONT_BIT									0x0002
#define NOTIFY_NICK_FLASH_BIT							0x0004
#define NOTIFY_CONT_FLASH_BIT							0x0008
#define NOTIFY_OTHER_FLASH_BIT						0x0010

#define C_TEXT											0
#define C_BACKGROUND									1
#define C_NAMES											2
#define C_NAMES_BACKGROUND								3
#define C_URL											4
#define C_SERVER										5
#define C_NOTICE										6
#define C_ACTION										7
#define C_QUIT											8
#define C_ERROR											9
#define C_NICK											10
#define C_MYNICK										11
#define C_JOIN											12
#define C_KICK											13
#define C_WHOIS											14
#define C_OP											15
#define C_VOICE											16
#define C_CTCP_REQ										17
#define C_CTCP_RPY										18
#define C_IGNORE										19
#define C_INPUT_BACKGROUND								20
#define C_INPUT											21

#define MAX_COLORS										22


#define F_TEXT												0
#define F_SERVER											1
#define F_URL												2
#define F_NAMES												3
#define F_INPUT												4

#define MAX_FONTS											5

#define E_JOIN												0
#define E_PART												1
#define E_NICK												2
#define E_QUIT												3
#define E_KICK												4
#define E_TOPIC												5
#define E_SNOTICE											6
#define E_UNOTICE											7
#define E_NOTIFY_ON											8
#define E_NOTIFY_OFF										9

#define MAX_EVENTS											10

#define CMD_QUIT											0
#define CMD_KICK											1
#define CMD_IGNORE											2
#define CMD_UNIGNORE										3
#define CMD_AWAY											4
#define CMD_BACK											5
#define CMD_UPTIME											6

#define MAX_COMMANDS										7

#define STATUS_OP_BIT									0x0001
#define STATUS_VOICE_BIT								0x0010
#define STATUS_NORMAL_BIT								0x0100
#define STATUS_IGNORE_BIT								0x1000

#define VERSION											"1.0.1"

#include <GraphicsDefs.h>

const int32 BIG_ENOUGH_FOR_A_REALLY_FAST_ETHERNET	= 1024 * 16;


// Application Messages
const uint32 M_ACTIVATION_CHANGE 					= 0x1011;
const uint32 M_NEW_CLIENT							= 0x1012;
const uint32 M_QUIT_CLIENT							= 0x1013;
const uint32 M_NEWS_CLIENT							= 0x1014;
const uint32 M_NICK_CLIENT							= 0x1015;
const uint32 M_ID_CHANGE							= 0x1016;

const uint32 M_NOTIFY_SELECT						= 0x1017;
const uint32 M_NOTIFY_PULSE							= 0x1018;
const uint32 M_NOTIFY_END							= 0x1019;
const uint32 M_NOTIFY_START							= 0x1020;
const uint32 M_NOTIFY_USER							= 0x1021;
const uint32 M_NOTIFY_COMMAND						= 0x1022;
const uint32 M_NOTIFY_WINDOW						= 0x1023;
const uint32 M_NOTIFY_SHUTDOWN						= 0x1024;

const uint32 M_LIST_BEGIN							= 0x1025;
const uint32 M_LIST_EVENT							= 0x1026;
const uint32 M_LIST_DONE							= 0x1027;
const uint32 M_LIST_COMMAND							= 0x1028;
const uint32 M_LIST_SHUTDOWN						= 0x1029;

const uint32 M_IS_IGNORED							= 0x1030;
const uint32 M_IGNORE_COMMAND						= 0x1031;
const uint32 M_IGNORE_SHUTDOWN						= 0x1032;
const uint32 M_UNIGNORE_COMMAND  					= 0x1033;
const uint32 M_EXCLUDE_COMMAND						= 0x1034;
const uint32 M_IGNORE_WINDOW						= 0x1035;

const uint32 M_STATE_CHANGE							= 0x1036;
const uint32 M_SERVER_STARTUP						= 0x1037;
const uint32 M_SERVER_CONNECTED						= 0x1038;

// Client Window Messages
const uint32 M_PREVIOUS_CLIENT						= 0x1300;
const uint32 M_NEXT_CLIENT							= 0x1301;
const uint32 M_PREVIOUS_INPUT						= 0x1302;
const uint32 M_NEXT_INPUT							= 0x1303;
const uint32 M_SUBMIT								= 0x1304;
const uint32 M_DISPLAY								= 0x1305;
const uint32 M_SUBMIT_RAW							= 0x1306;
const uint32 M_CHANNEL_MSG							= 0x1307;
const uint32 M_CHANGE_NICK							= 0x1308;
const uint32 M_CHANNEL_MODES						= 0x1309;

const uint32 M_HISTORY_SELECT						= 0x1311;
const uint32 M_SETUP_ACTIVATE						= 0x1312;
const uint32 M_SCROLL_TOGGLE						= 0x1313;
const uint32 M_SETUP_HIDE							= 0x1314;

// Server Messages
const uint32 M_PARSE_LINE							= 0x1700;
const uint32 M_SERVER_SEND							= 0x1701;
const uint32 M_SERVER_SHUTDOWN						= 0x1702;
const uint32 M_CLIENT_SHUTDOWN						= 0x1703;
const uint32 M_IGNORED_PRIVMSG						= 0x1704;
const uint32 M_LAG_CHANGED							= 0x1705;
const uint32 M_SERVER_DISCONNECT					= 0x1706;
const uint32 M_REJOIN_ALL							= 0x1707;
const uint32 M_SLASH_RECONNECT						= 0x1708;

// List Messages
const uint32 M_SORT_CHANNEL							= 0x1800;
const uint32 M_SORT_USER							= 0x1801;
const uint32 M_FILTER_LIST							= 0x1802;
const uint32 M_LIST_FIND							= 0x1803;
const uint32 M_LIST_FAGAIN							= 0x1804;
const uint32 M_LIST_INVOKE							= 0x1805;

// Ignore Messages
const uint32 M_IGNORE_ADD							= 0x1900;
const uint32 M_IGNORE_EXCLUDE						= 0x1901;
const uint32 M_IGNORE_REMOVE						= 0x1902;
const uint32 M_IGNORE_CLEAR							= 0x1903;

// Notify Messages
const uint32 M_NOTIFY_ADD							= 0x1950;
const uint32 M_NOTIFY_REMOVE						= 0x1951;
const uint32 M_NOTIFY_CLEAR							= 0x1952;

// Channel Messages
const uint32 M_USER_QUIT							= 0x1600;
const uint32 M_USER_ADD								= 0x1601;
const uint32 M_CHANNEL_NAMES						= 0x1602;
const uint32 M_CHANNEL_TOPIC						= 0x1603;
const uint32 M_CHANNEL_MODE							= 0x1604;
const uint32 M_INPUT_FOCUS							= 0x1605;
const uint32 M_CHANNEL_GOT_KICKED					= 0x1606;
const uint32 M_REJOIN								= 0x1607;

// Setup Window
const uint32 M_SERVER_SELECT						= 0x1050;
const uint32 M_SERVER_NEW							= 0x1051;
const uint32 M_SERVER_COPY							= 0x1052;
const uint32 M_SERVER_REMOVE						= 0x1053;
const uint32 M_SERVER_OPTIONS						= 0x1054;
const uint32 M_SERVER_NAME							= 0x1055;
const uint32 M_DONE									= 0x1056;
const uint32 M_CANCEL								= 0x1057;
const uint32 M_PREFS_SHUTDOWN						= 0x1058;
const uint32 M_SETUP_BUTTON							= 0x1059;
const uint32 M_PREFS_BUTTON							= 0x1060;
const uint32 M_NICK_ADD								= 0x1061;
const uint32 M_NICK_REMOVE							= 0x1062;
const uint32 M_NICK_LOAD							= 0x1063;
const uint32 M_LAUNCH_WEB							= 0x1064;


// Server Options
const uint32 M_SOPTIONS_MOTD						= 0x2000;
const uint32 M_SOPTIONS_IDENTD						= 0x2001;
const uint32 M_SOPTIONS_SHUTDOWN					= 0x2002;
const uint32 M_SOPTIONS_CONNECT_ON_STARTUP			= 0x2003;

// Preferences
const uint32 M_PREFERENCE_GROUP						= 0x1400;
const uint32 M_WINDOW_FOLLOWS						= 0x1406;
const uint32 M_FONT_CHANGE							= 0x1407;
const uint32 M_FONT_SIZE_CHANGE						= 0x1408;
const uint32 M_NOTIFICATION							= 0x1409;
const uint32 M_NOTIFY_NONE							= 0x1410;
const uint32 M_NOTIFY_NICK							= 0x1411;
const uint32 M_NOTIFY_CONTENT						= 0x1412;
const uint32 M_NOTIFY_COMBO   						= 0x1413;
const uint32 M_NOTIFY_NICK_FLASH					= 0x1414;
const uint32 M_NOTIFY_CONT_FLASH 					= 0x1415;
const uint32 M_NOTIFY_OTHER_FLASH 					= 0x1416;
const uint32 M_AKA_MODIFIED							= 0x1417;
const uint32 M_OTHER_NICK_MODIFIED					= 0x1418;
const uint32 M_AUTO_NICK_MODIFIED					= 0x1419;


// Color (Part of Preferinces)
const uint32 M_COLOR_INVOKE							= 0x1500;
const uint32 M_COLOR_SELECT							= 0x1501;
const uint32 M_COLOR_CHANGE							= 0x1502;


const uint32 INVITE_USER							= 0x1134; // not inuse yet

const uint32 SEND_ACTION							= 0x1200;
const uint32 CHAT_ACTION							= 0x1201;
const uint32 CHOSE_FILE								= 0x1202;
const uint32 OPEN_MWINDOW							= 0x1203;
const uint32 CYCLE_WINDOWS							= 0x1204;
const uint32 CYCLE_BACK								= 0x1205;
const uint32 DCC_ACCEPT								= 0x1206;
const uint32 CHAT_ACCEPT							= 0x1207;
const uint32 M_DCC_PORT								= 0x1208;
const uint32 M_DCC_FILE_WIN							= 0x1209;
const uint32 M_DCC_FILE_WIN_DONE					= 0x1210;
const uint32 M_ADD_RESUME_DATA						= 0x1211;
const uint32 M_DCC_MESSENGER						= 0x1212;
const uint32 M_DCC_COMPLETE							= 0x1213;
const uint32 POPUP_MODE								= 0x1214;
const uint32 POPUP_CTCP								= 0x1215;
const uint32 POPUP_WHOIS							= 0x1216;
const uint32 POPUP_KICK								= 0x1217;

#ifdef DEV_BUILD
extern bool DumpReceived;
extern bool DumpSent;
extern bool ViewCodes;
extern bool ShowId;
extern bool NotifyStatus;
extern bool NotifyReuse;
#endif

#endif
