ALLDIRS=dial gauge tzclock

all: $(ALLDIRS)
	$(MAKE) -C dial
	$(MAKE) -C gauge
	$(MAKE) -C tzclock

clean: $(ALLDIRS)
	$(MAKE) -C dial clean
	$(MAKE) -C gauge clean
	$(MAKE) -C tzclock clean

install: $(ALLDIRS)
	$(MAKE) -C dial install
	$(MAKE) -C gauge install
	$(MAKE) -C tzclock install

dist: $(ALLDIRS)
	$(MAKE) -C dial dist
	$(MAKE) -C gauge dist
	$(MAKE) -C tzclock dist

distclean: $(ALLDIRS)
	$(MAKE) -C dial distclean
	$(MAKE) -C gauge distclean
	$(MAKE) -C tzclock distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C dial maintainer-clean
	$(MAKE) -C gauge maintainer-clean
	$(MAKE) -C tzclock maintainer-clean

