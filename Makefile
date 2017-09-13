ALLDIRS= dial gauge tzclock

all: $(ALLDIRS)
	$(MAKE) -C dial
	$(MAKE) -C gauge
	$(MAKE) -C tzclock

clean: $(ALLDIRS)
	$(MAKE) -C dial clean
	$(MAKE) -C gauge clean
	$(MAKE) -C tzclock clean

dist: $(ALLDIRS)
	$(MAKE) -C dial dist
	$(MAKE) -C gauge dist
	$(MAKE) -C tzclock dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C dial dist-bzip2
	$(MAKE) -C gauge dist-bzip2
	$(MAKE) -C tzclock dist-bzip2

install: $(ALLDIRS)

installx: $(ALLDIRS)
	$(MAKE) -C dial install
	$(MAKE) -C gauge install
	$(MAKE) -C tzclock install

distclean: $(ALLDIRS)
	$(MAKE) -C dial distclean
	$(MAKE) -C gauge distclean
	$(MAKE) -C tzclock distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C dial maintainer-clean
	$(MAKE) -C gauge maintainer-clean
	$(MAKE) -C tzclock maintainer-clean
