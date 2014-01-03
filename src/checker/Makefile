ROOT := ../..
include $(ROOT)/*.mk

all: checker

checker: checker.o checker_server.o main.o resource_manager.o ../common/config.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) $(GENCLEAN) checker 

.PHONY: all clean
