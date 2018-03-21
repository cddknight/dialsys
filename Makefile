ALLDIRS= libdial gauge tzclock

all: $(ALLDIRS)
	$(MAKE) -C libdial
	$(MAKE) -C gauge
	$(MAKE) -C tzclock

clean: $(ALLDIRS)
	$(MAKE) -C libdial clean
	$(MAKE) -C gauge clean
	$(MAKE) -C tzclock clean

dist: $(ALLDIRS)
	$(MAKE) -C libdial dist
	$(MAKE) -C gauge dist
	$(MAKE) -C tzclock dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C libdial dist-bzip2
	$(MAKE) -C gauge dist-bzip2
	$(MAKE) -C tzclock dist-bzip2

install: $(ALLDIRS)

installx: $(ALLDIRS)
	$(MAKE) -C libdial install
	$(MAKE) -C gauge install
	$(MAKE) -C tzclock install

distclean: $(ALLDIRS)
	$(MAKE) -C libdial distclean
	$(MAKE) -C gauge distclean
	$(MAKE) -C tzclock distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C libdial maintainer-clean
	$(MAKE) -C gauge maintainer-clean
	$(MAKE) -C tzclock maintainer-clean
