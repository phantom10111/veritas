export

PWD := $(shell pwd)

override CXXFLAGS += -std=gnu++11 -I$(PWD)/include -O2
override LDLIBS += -lboost_system -lpqxx
GENCLEAN := *.o
CLEAN := src/checker/checker

all: checker client webserver

checker:
	cd src/checker && $(MAKE)

client:
	cd src/client && $(MAKE)

webserver:
	cd src/webserver && $(MAKE)

clean:
	cd src/checker && rm -rf $(GENCLEAN)
	cd src/client && rm -rf $(GENCLEAN)
	cd src/common && rm -rf $(GENCLEAN)
	cd src/webserver && rm -rf $(GENCLEAN)
	rm -rf $(CLEAN)

.PHONY: all checker client webserver
