ROOT := ../..
include $(ROOT)/*.mk

all: checker

checker: checker.o checker_server.o main.o resource_manager.o ../common/config.o ../common/ssl_socket.o

clean:
	$(RMALL) checker 

.PHONY: all clean
