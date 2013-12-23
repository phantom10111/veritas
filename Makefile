all: checker client webserver

checker:
	cd src/checker && $(MAKE)

client:
	cd src/client && $(MAKE)

webserver:
	cd src/webserver && $(MAKE)

.PHONY: all checker client webserver
