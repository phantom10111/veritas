ROOT := ../..
include $(ROOT)/*.mk

all: webserver

webserver: main.o webserver.o ../common/config.o

clean:
	$(RMALL) webserver

.PHONY: all clean
