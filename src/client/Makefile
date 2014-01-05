ROOT := ../..
include $(ROOT)/*.mk

all: client

client: main.o ../common/config.o ../common/ssl_socket.o

clean:
	$(RMALL) client

.PHONY: all clean
