ROOT := ../..
include $(ROOT)/*.mk

all: webserver_main

webserver_main: webserver_main.o webserver.o ../common/config.o

clean:
	$(RMALL) webserver_main

.PHONY: all clean
