
SRC=AboutWindow.cpp \
	Bowser.cpp \
	ChannelWindow.cpp \
	ClientFilter.cpp \
	ClientWindow.cpp \
	ColorLabel.cpp \
	DCCFileWindow.cpp \
	DCCHandler.cpp \
	HistoryMenu.cpp \
	IgnoreWindow.cpp \
	IRCView.cpp \
	ListWindow.cpp \
	MessageWindow.cpp \
	NameItem.cpp \
	NamesView.cpp \
	Notify.cpp \
	NotifyWindow.cpp \
	ParseCTCP.cpp \
	ParseENums.cpp \
	ParseEvents.cpp \
	PrefColor.cpp \
	PrefCommand.cpp \
	PrefEvent.cpp \
	PrefFont.cpp \
	PrefGeneral.cpp \
	PrefNotify.cpp \
	PrefsFrame.cpp \
	Prompt.cpp \
	ServerOptions.cpp \
	ServerWindow.cpp \
	Settings.cpp \
	SetupWindow.cpp \
	StatusView.cpp \
	StringManip.cpp

RVSRC=RunView.cpp \
	RunTest.cpp \
	IRCState.cpp

define SRC_TO_OBJ
	$(addprefix objs/, $(addsuffix .o, $(foreach file, $(SRC), \
	$(basename $(notdir $(file))))))
endef

define RVSRC_TO_OBJ
	$(addprefix objs/, $(addsuffix .o, $(foreach file, $(RVSRC), \
	$(basename $(notdir $(file))))))
endef
	

OBJS=$(SRC_TO_OBJ)
RVOBJS=$(RVSRC_TO_OBJ)

LIBS=-lbe -lnet -lnetapi -ltextencoding -ltranslation -ltracker

OPTIMIZE=-O3
#DEBUG=-g

CXXFLAGS=$(OPTIMIZE) $(DEBUG) -Wall -Wno-multichar -Wno-ctor-dtor-privacy
LDFLAGS=

all : objs Bowser rv

Bowser : $(OBJS) Bowser.rsrc
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	xres -o $@ Bowser.rsrc
	mimeset -f $@

rv : $(RVOBJS)
	$(CXX) $(LDFLAGS) -o $@ $(RVOBJS) -lbe

objs/%.o : %.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

objs::
	@[ -d objs ] || mkdir objs > /dev/null

clean:
	@[ -d objs ] && rm -r objs/* > /dev/null


objs/AboutWindow.o : AboutWindow.cpp AboutWindow.h
objs/Bowser.o: Bowser.cpp ServerWindow.h ClientWindow.h Settings.h Notify.h \
 SetupWindow.h IgnoreWindow.h Prompt.h Bowser.h IRCDefines.h
objs/ChannelWindow.o: ChannelWindow.cpp Names.h Bowser.h IRCDefines.h \
 IRCView.h StatusView.h Settings.h StringManip.h IgnoreWindow.h \
 Prompt.h ChannelWindow.h ClientWindow.h
objs/ClientFilter.o: ClientFilter.cpp IRCDefines.h ClientWindow.h \
 ClientFilter.h
objs/ClientWindow.o: ClientWindow.cpp Bowser.h IRCDefines.h MessageWindow.h \
 ClientWindow.h HistoryMenu.h IRCView.h StringManip.h Settings.h \
 StatusView.h ServerWindow.h ClientFilter.h
objs/ColorLabel.o: ColorLabel.cpp ColorLabel.h IRCDefines.h
objs/DCCFileWindow.o: DCCFileWindow.cpp IRCDefines.h Bowser.h \
 ServerWindow.h ClientWindow.h DCCFileWindow.h
objs/DCCHandler.o: DCCHandler.cpp IRCDefines.h ServerWindow.h \
 ClientWindow.h
objs/HistoryMenu.o: HistoryMenu.cpp IRCDefines.h HistoryMenu.h
objs/IRCState.o: IRCState.cpp RunView.h IRCDefines.h IRCState.h
objs/IRCView.o: IRCView.cpp Bowser.h IRCDefines.h ClientWindow.h \
 StringManip.h IRCView.h
objs/IgnoreWindow.o: IgnoreWindow.cpp IRCDefines.h Prompt.h StatusView.h \
 Settings.h Bowser.h IgnoreWindow.h
objs/ListWindow.o: ListWindow.cpp IRCDefines.h Prompt.h StatusView.h \
 Settings.h Bowser.h ListWindow.h
objs/MessageWindow.o: MessageWindow.cpp IRCView.h StringManip.h \
 IRCDefines.h ServerWindow.h ClientWindow.h StatusView.h \
 MessageWindow.h
objs/NameItem.o: NameItem.cpp IRCDefines.h Names.h
objs/NamesView.o: NamesView.cpp Bowser.h IRCDefines.h Names.h
objs/Notify.o: Notify.cpp IRCDefines.h Notify.h
objs/NotifyWindow.o: NotifyWindow.cpp IRCDefines.h Prompt.h StatusView.h \
 Settings.h Bowser.h NotifyWindow.h
objs/ParseCTCP.o: ParseCTCP.cpp Bowser.h IRCDefines.h StringManip.h \
 ServerWindow.h ClientWindow.h
objs/ParseENums.o: ParseENums.cpp IRCDefines.h ListWindow.h Bowser.h \
 IgnoreWindow.h Prompt.h StringManip.h StatusView.h ServerWindow.h \
 ClientWindow.h NotifyWindow.h
objs/ParseEvents.o: ParseEvents.cpp IRCDefines.h Bowser.h ChannelWindow.h \
 ClientWindow.h MessageWindow.h StringManip.h IgnoreWindow.h Prompt.h \
 StatusView.h ServerWindow.h
objs/PrefColor.o: PrefColor.cpp Bowser.h IRCDefines.h ColorLabel.h \
 PrefColor.h
objs/PrefCommand.o: PrefCommand.cpp Bowser.h IRCDefines.h PrefCommand.h
objs/PrefEvent.o: PrefEvent.cpp Bowser.h IRCDefines.h PrefEvent.h
objs/PrefFont.o: PrefFont.cpp Bowser.h IRCDefines.h PrefFont.h
objs/PrefGeneral.o: PrefGeneral.cpp IRCDefines.h Bowser.h PrefGeneral.h
objs/PrefNotify.o: PrefNotify.cpp Bowser.h IRCDefines.h PrefNotify.h
objs/PrefsFrame.o: PrefsFrame.cpp IRCDefines.h Settings.h PrefsFrame.h
objs/Prompt.o: Prompt.cpp Prompt.h
objs/RunTest.o: RunTest.cpp RunView.h IRCDefines.h
objs/RunView.o: RunView.cpp IRCState.h RunView.h IRCDefines.h
objs/ServerOptions.o: ServerOptions.cpp IRCDefines.h SetupWindow.h \
 Settings.h ServerOptions.h
objs/ServerWindow.o: ServerWindow.cpp Bowser.h IRCDefines.h IRCView.h \
 StringManip.h ChannelWindow.h ClientWindow.h DCCFileWindow.h \
 MessageWindow.h ListWindow.h IgnoreWindow.h Prompt.h NotifyWindow.h \
 StatusView.h ServerWindow.h
objs/Settings.o: Settings.cpp IRCDefines.h Settings.h
objs/SetupWindow.o: SetupWindow.cpp Bowser.h IRCDefines.h Prompt.h \
 ServerWindow.h ClientWindow.h PrefsFrame.h PrefGeneral.h PrefNotify.h \
 PrefFont.h PrefColor.h PrefEvent.h PrefCommand.h ServerOptions.h \
 IgnoreWindow.h SetupWindow.h
objs/StatusView.o: StatusView.cpp ClientWindow.h StatusView.h
objs/StringManip.o: StringManip.cpp Bowser.h IRCDefines.h
