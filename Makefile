#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IMPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = upnp
COMMON = common.h

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).cpp | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:
### added '-D__STDC_CONSTANT_MACROS' to compiler options for new ffmpeg-abi

CXX      ?= gcc
CXXFLAGS ?= -fPIC -g -Wall -O2 -Wextra -Woverloaded-virtual -Wno-parentheses -D__STDC_CONSTANT_MACROS -Wl,-R/usr/local/lib 

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

LIBS += -lupnp -lixml -lsqlite3 -lavformat -lavcodec

INCLUDES += -I$(VDRDIR)/include	-I/usr/include -Iinc \

ifdef DVBDIR
	INCLUDES += -I$(DVBDIR)
endif

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
DEFINES += -D__STDC_CONSTANT_MACROS 

ifdef WITH_WINDOWS_MEDIA
	DEFINES += -DWITH_WINDOWS_MEDIA
endif

### DEFINES += -DWITHOUT_AUDIO -DWITHOUT_CUSTOM_VIDEOS
DEFINES += -DWITHOUT_CUSTOM_VIDEOS

### With the debian linux wheezy distribution a ffmpeg library is used having the version major flag 
### of libavcodec and libavformat set to 53. In this case add the following line: 
#DEFINES += -DAVCODEC53

### The object files (add further files here):

# DLNA Profiles
DLNA_PROFILES =	dlna/profiles/aac.o \
		dlna/profiles/ac3.o \
		dlna/profiles/amr.o \
		dlna/profiles/atrac3plus.o \
		dlna/profiles/jpeg.o \
		dlna/profiles/lpcm.o \
		dlna/profiles/mpa.o \
		dlna/profiles/mpeg1.o \
		dlna/profiles/mpeg2.o \
		dlna/profiles/mpeg4_p2.o \
		dlna/profiles/mpeg4_p10.o \
		dlna/profiles/png.o \
		dlna/profiles/container.o

# Windows Media Formats
ifdef WITH_WINDOWS_MEDIA
	DLNA_PROFILES +=	dlna/profiles/wma.o \
				dlna/profiles/wmv9.o
endif

DLNA_OBJS =	dlna/dlna.o \
		dlna/avdetector.o \
		$(DLNA_PROFILES)

# Root folder
OBJS = $(PLUGIN).o \
		common.o \
		misc/menusetup.o \
		misc/util.o \
		misc/config.o \
		misc/search.o \
		misc/vdrepg.o \
		misc/split.o \
		database/database.o \
		database/metadata.o \
		database/object.o \
		database/mediator.o \
		database/resources.o \
		server/server.o \
		server/webserver.o \
		upnp/service.o \
		upnp/connectionmanager.o \
		upnp/contentdirectory.o \
		receiver/livereceiver.o \
		receiver/recplayer.o \
		receiver/fileplayer.o \
		$(DLNA_OBJS)

### The main target:

all: libvdr-$(PLUGIN).so i18n

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $(LIBS) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies

$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.cpp)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -k_ -ktr -ktrNOOP --msgid-bugs-address='<see README>' -o $@ $(OBJS:%.o=%.cpp)

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs) $(I18Npot)

### Targets:

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS) -shared $(OBJS) -o $@ -lc
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar --exclude="doc" -czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ $(PODIR)/*.mo $(PODIR)/*.pot
