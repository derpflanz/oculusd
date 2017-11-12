include ../Makefile.config

install: oculusd.1 oculusd.conf.5
	$(CP) oculusd.1 $(MANPAGE_PATH)1
	$(CP) oculusd.conf.5 $(MANPAGE_PATH)5

uninstall: 
	$(RM) $(MANPAGE_PATH)1/oculusd.1
	$(RM) $(MANPAGE_PATH)5/oculusd.conf.5

