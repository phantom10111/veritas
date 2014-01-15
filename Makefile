all: checker client webserver

checker: privkey.pem server.pem
	cd src/checker && $(MAKE)
	cp src/checker/checker .

client: server.pem
	cd src/client && $(MAKE)
	cp src/client/client .

webserver: privkey.pem server.pem
	cd src/webserver && $(MAKE)
	cp src/webserver/webserver .

privkey.pem:
	openssl genrsa -out privkey.pem 4096
	chmod og-rwx privkey.pem

server.pem: privkey.pem
	openssl req -new -x509 -key privkey.pem -outform PEM -out server.pem -subj / -days 1095

clean:
	cd src/checker && $(MAKE) clean
	cd src/client && $(MAKE) clean
	cd src/common && $(MAKE) clean
	cd src/webserver && $(MAKE) clean
	$(RM) checker client webserver privkey.pem server.pem

.PHONY: all checker client webserver clean
