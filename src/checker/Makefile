all: checker

checker: checker.o checker_server.o main.o resource_manager.o ../common/config.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: all
