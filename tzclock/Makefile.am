AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = tzclock screenSize
tzclock_SOURCES = src/TzClock.c src/TzClockCairo.c src/ParseZone.c src/TzClockDisp.h src/TimeZone.h \
		src/ParseZone.h src/TzClockIcon.xpm src/TzClockIcon_small.xpm
screenSize_SOURCES = src/screenSize.c
AM_CPPFLAGS = $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
EXTRA_DIST = tzclock.desktop tzclock.appdata.xml icons/48x48/tzclock.png icons/128x128/tzclock.png \
		icons/scalable/tzclock.svg tzclock.man COPYING AUTHORS
Applicationsdir = $(datadir)/applications
Applications_DATA = tzclock.desktop 
AppDatadir = $(datadir)/appdata
AppData_DATA = tzclock.appdata.xml 
Icondir = $(datadir)/icons/hicolor/scalable/apps
Icon_DATA = icons/scalable/tzclock.svg
Icon128dir = $(datadir)/icons/hicolor/128x128/apps
Icon128_DATA = icons/128x128/tzclock.png
Icon48dir = $(datadir)/icons/hicolor/48x48/apps
Icon48_DATA = icons/48x48/tzclock.png
man1_MANS = tzclock.man

