ROOT := ../..
include $(ROOT)/*.mk

all: webserver

webserver: main.o webserver.o commands.o ../common/config.o ../common/ssl_socket.o

clean:
	$(RMALL) webserver

.PHONY: all clean
