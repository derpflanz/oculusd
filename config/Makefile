include ../Makefile.config

install: oculusd.dtd oculusd.conf
	$(MKDIR) $(CONFIG_PATH)
	$(CP) -i oculusd.conf $(CONFIG_PATH)/oculusd.conf
	$(CP) oculusd.dtd $(DTD_PATH)
	
uninstall:
	$(RMDIR) $(CONFIG_PATH)
	$(RMDIR) $(DTD_PATH)
