CXXFLAGS := -O2
COMPILE_OPTIONS := -std=gnu++11 -I$(ROOT)/include -MMD
LINK_OPTIONS := -lboost_system -lpqxx

RM := rm -rf
GENCLEAN := *.o *.d
RMALL = $(RM) $(GENCLEAN)

.DEFAULT_GOAL := all

%.o: %.cpp
	$(COMPILE.cpp) $(COMPILE_OPTIONS) $(OUTPUT_OPTION) $<

%:
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) $(LINK_OPTIONS) -o $@

%.d:

.PHONY: %.d

-include *.d ../common/*.d
