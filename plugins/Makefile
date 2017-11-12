include ../Makefile.config

PLUGINS=libocp_default.so libocp_process.so libocp_disk.so libocp_alarm.so
PLUGIN_DTDS=process.dtd
PLUGIN_CONFIGS=process.conf

all: libocp_default.so libocp_process.so libocp_disk.so libocp_alarm.so

libocp_default.so: libocp_default.c libocp_default.h
	$(GCC) $(PLUGIN_GCC_FLAGS) $(PLUGIN_LD_FLAGS) -o libocp_default.so libocp_default.c
				
libocp_process.so: libocp_process.c libocp_process.h
	$(GCC) $(PLUGIN_GCC_FLAGS) $(PLUGIN_LD_FLAGS) -o libocp_process.so libocp_process.c

libocp_disk.so: libocp_disk.c libocp_disk.h
	$(GCC) $(PLUGIN_GCC_FLAGS) $(PLUGIN_LD_FLAGS) -o libocp_disk.so libocp_disk.c

libocp_alarm.so: libocp_alarm.c libocp_alarm.h
	$(GCC) $(PLUGIN_GCC_FLAGS) $(PLUGIN_LD_FLAGS) -o libocp_alarm.so libocp_alarm.c

clean:
	$(RM) $(PLUGINS)

install: all
	$(MKDIR) $(DTD_PATH)
	$(MKDIR) $(PLUGIN_PATH)
	$(CP) $(PLUGINS) $(PLUGIN_PATH)
	$(CP) -t $(DTD_PATH) $(PLUGIN_DTDS)
	$(CP) -i -t $(CONFIG_PATH) $(PLUGIN_CONFIGS)

uninstall:
	$(RMDIR) $(PLUGIN_PATH)
