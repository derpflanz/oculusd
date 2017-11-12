include ../Makefile.config

LIBRARY=liboculus.so
BINARY=oculusd

BINARY_LD_FLAGS=-lxml2 -L./ -loculus -lpcre -lpthread -ldl
LIB_LD_FLAGS=

BINARY_SRCS=command.c config.c main.c network.c options.c plugin.c sighndlr.c monitor.c
BINARY_HDRS=command.h config.h globals.h network.h options.h plugin.h sighndlr.h monitor.h

LIBRARY_SRCS=liboculus.c action.c event.c
LIBRARY_HDRS=liboculus.h $(BINARY_HDRS) action.h event.h

all: $(LIBRARY) $(BINARY)

$(BINARY): $(BINARY_SRCS) $(BINARY_HDRS)
	@echo
	@echo "**** Building $(BINARY) ****"
	$(GCC) $(BINARY_GCC_FLAGS) $(BINARY_LD_FLAGS) $(BINARY_SRCS) -o $(BINARY)

$(LIBRARY): $(LIBRARY_SRCS) $(LIBRARY_HDRS)
	@echo
	@echo "**** Building $(LIBRARY) *****"
	$(GCC) $(LIBRARY_GCC_FLAGS) $(LIBRARY_LD_FLAGS) $(LIBRARY_SRCS) -o $(LIBRARY)

install: $(LIBRARY) $(BINARY)
	$(MKDIR) -v -p $(DTD_PATH)
	$(CP) $(LIBRARY) $(LIBRARY_PATH)
	$(CP) $(BINARY) $(BINARY_PATH)

uninstall:
	$(RM) $(LIBRARY_PATH)/$(LIBRARY)
	$(RM) $(BINARY_PATH)/$(BINARY)

clean:
	@echo
	@echo "**** Cleaning up ****"
	$(RM) *.o
	$(RM) *~
	$(RM) $(BINARY)
	$(RM) $(LIBRARY)
