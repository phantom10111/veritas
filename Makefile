export

PWD := $(shell pwd)

override CXXFLAGS += -std=gnu++11 -I$(PWD)/include -O2
override LDLIBS += -lboost_system -lpqxx
RM = rm -rf
GENCLEAN := *.o

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
