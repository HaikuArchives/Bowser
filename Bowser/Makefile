################################################
#
#	Bowser
#
################################################

# the file name, ...

BINARY := Bowser
VERSION := d42b
LIBS := -lbe -lnet -lnetapi -ltextencoding -ltranslation -ltracker

################################################

# output directories

OBJDIR := obj
DEPDIR := dep
SRCDIR := .
DATADIR := data

# compiler, linker, ...

CC := g++ -c -pipe
CFLAGS :=
CFLAGS += -O3
CFLAGS += -Wall -Wno-multichar -Wno-ctor-dtor-privacy

CFLAGS.default :=
CFLAGS.default := -march=pentium
CFLAGS.default += -mcpu=pentium
CFLAGS.default += -DVERSION_STRING=\"$(VERSION)\" -DBINARY_NAME=\"$(BINARY)\"

LD := g++
LDFLAGS := $(LIBS)

DEP := g++ -MM

ZIP := zip -r -9 -y

################################################

# the engine

MAKEFILE := makefile

FULLNAME := $(subst \ ,_,$(BINARY))-$(VERSION)

DATA := $(shell find $(DATADIR) -type f)

EXTRADATA:= $(wildcard data.zip)

BASESOURCES := $(shell cd $(SRCDIR) && ls -1 *.cpp)
SOURCES := $(addprefix $(SRCDIR)/,$(BASESOURCES))
OBJECTS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(BASESOURCES))))
DEPENDS := $(addprefix $(DEPDIR)/,$(addsuffix .d,$(basename $(BASESOURCES))))

BASEHEADERS := $(shell cd $(SRCDIR) && ls -1 *.h 2> /dev/null)
HEADERS := $(addprefix $(SRCDIR)/,$(BASEHEADERS))

BINARIES := $(BINARY)
OBJDIRS := $(OBJDIR)

.PHONY : default clean binarchive sourcearchive all

.DELETE_ON_ERROR : $(BINARIES)

default : $(BINARIES)

clean :
	@echo cleaning
	@rm -rf $(BINARIES) $(OBJDIRS) $(DEPDIR) $(filter-out data.zip,$(wildcard *.zip)) *.zip~

all : sourcearchive binarchive

sourcearchive : $(FULLNAME)-src.zip

binarchive : $(FULLNAME)-$(BE_HOST_CPU).zip


$(BINARY) : $(OBJECTS)
	@echo linking $@
	@$(LD) $(LDFLAGS) $^ -o "$@"
	@xres -o $@ $(SRCDIR)/Bowser.rsrc
	@mimeset -f $@

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@echo compiling $@
	@mkdir -p $(OBJDIR)
	@$(CC) $< $(CFLAGS) $(CFLAGS.default) -o $@

$(DEPDIR)/%.d : $(SRCDIR)/%.cpp
	@echo generating dependencies for $<
	@mkdir -p $(DEPDIR)
	@$(DEP) $< > $@
	@echo -e yf P0yf.i$(OBJDIR)/\\033f a$(OBJDIR.C)/\\033t:a$(DEPDIR)/\\033pad \\033f:a $(MAKEFILE)\\033ZZ | vi $@ >/dev/null 2>/dev/null

$(FULLNAME)-$(BE_HOST_CPU).zip : $(BINARIES) $(DATA) $(MAKEFILE) $(EXTRADATA)
	@rm -rf $@~
	@mkdir -p $@~/$(FULLNAME)-$(BE_HOST_CPU)/
	@copyattr -d -r $(BINARIES) $(DATADIR)/* $@~/$(FULLNAME)-$(BE_HOST_CPU)/
ifneq "$(EXTRADATA)" ""
	@unzip $(EXTRADATA) -d $@~/$(FULLNAME)-$(BE_HOST_CPU)/
endif
	@find $@~ -type f | xargs mimeset
	@cd $@~ && $(ZIP) $@ $(FULLNAME)-$(BE_HOST_CPU)
	@mv -f $@~/$@ .
	@rm -rf $@~

$(FULLNAME)-src.zip : $(SOURCES) $(HEADERS) $(DATA) $(MAKEFILE) $(EXTRADATA)
	@rm -rf $@~
	@mkdir -p $@~/$(FULLNAME)-src/
	@copyattr -d -r $(SRCDIR) $(DATADIR) $(MAKEFILE) $@~/$(FULLNAME)-src/
ifneq "$(EXTRADATA)" ""
	@copyattr -d -r $(EXTRADATA) $@~/$(FULLNAME)-src/
endif
	@find $@~ -type f | xargs mimeset
	@cd $@~ && $(ZIP) $@ $(FULLNAME)-src
	@mv -f $@~/$@ .
	@rm -rf $@~

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),depend)
ifneq ($(MAKECMDGOALS),sourcearchive)
include $(DEPENDS)
endif
endif
endif
