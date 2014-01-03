all: checker client webserver

checker:
	cd src/checker && $(MAKE)

client:
	cd src/client && $(MAKE)

webserver:
	cd src/webserver && $(MAKE)

clean:
	cd src/checker && make clean
	cd src/client && make clean
	cd src/common && make clean
	cd src/webserver && make clean

.PHONY: all checker client webserver clean
