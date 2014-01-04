CXXFLAGS := -O2

override COMPILE_OPTIONS := -std=gnu++11 -I$(ROOT)/include
override LINK_OPTIONS := -lboost_system -lpqxx

RMALL = $(RM) *.o *.d


.DEFAULT_GOAL := all

%.o: %.cpp
	$(COMPILE.cpp) $(COMPILE_OPTIONS) $(OUTPUT_OPTION) $<

%:
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) $(LINK_OPTIONS) $(OUTPUT_OPTION)


ifneq ($(TRACK_DEPENDENCIES), 0)

override COMPILE_OPTIONS += -MD

%.d: ;

.PHONY: %.d

-include *.d ../common/*.d

endif
