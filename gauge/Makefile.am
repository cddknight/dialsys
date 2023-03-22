AUTOMAKE_OPTIONS = dist-bzip2
bin_PROGRAMS = gauge
gauge_SOURCES = src/Gauge.c src/GaugeCPU.c src/GaugeSensors.c src/GaugeWeather.c \
		src/GaugeMemory.c src/GaugeBattery.c src/GaugeNetwork.c src/GaugeEntropy.c \
		src/GaugeTide.c src/GaugeHarddisk.c src/GaugeThermo.c src/GaugePower.c \
		src/GaugeMoon.c src/GaugeWifi.c src/GaugeCairo.c src/GaugeDisp.h src/socketC.c \
		src/socketC.h buildDate.h src/GaugeIcon.xpm src/GaugeIcon_small.xpm 
gauge_CPPFLAGS = -D_FILE_OFFSET_BITS=64 $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
EXTRA_DIST = gauge.desktop icons/48x48/gauge.png icons/128x128/gauge.png icons/scalable/gauge.svg \
		COPYING AUTHORS
Applicationsdir = $(datadir)/applications
Applications_DATA = gauge.desktop
Icondir = $(datadir)/icons/hicolor/scalable/apps
Icon_DATA = icons/scalable/gauge.svg
Icon128dir = $(datadir)/icons/hicolor/128x128/apps
Icon128_DATA = icons/128x128/gauge.png
Icon48dir = $(datadir)/icons/hicolor/48x48/apps
Icon48_DATA = icons/48x48/gauge.png
BUILT_SOURCES = buildDate.h
CLEANFILES = buildDate.h
buildDate.h:
	setBuildDate -c
