all: checker client webserver

checker:
	cd src/checker && $(MAKE)
	cp src/checker/checker .

client:
	cd src/client && $(MAKE)
	cp src/client/client .

webserver:
	cd src/webserver && $(MAKE)
	cp src/webserver/webserver .

clean:
	cd src/checker && $(MAKE) clean
	cd src/client && $(MAKE) clean
	cd src/common && $(MAKE) clean
	cd src/webserver && $(MAKE) clean
	$(RM) checker client webserver

.PHONY: all clean
