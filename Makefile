all:
	make -C src/ all
	make -C plugins/ all

clean:
	make -C src/ clean
	make -C plugins/ clean

install:
	make -C config/ install
	make -C src/ install
	make -C plugins/ install
	make -C man/ install

uninstall:
	make -C src/ uninstall
	make -C plugins/ uninstall
	make -C config/ uninstall
	make -C man/ uninstall
	
