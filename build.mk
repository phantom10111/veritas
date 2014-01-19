CXXFLAGS := -O2

override COMPILE_OPTIONS := -std=gnu++11 -I$(ROOT)/include
override LINK_OPTIONS := -L/usr/local/lib -Wl,-rpath /usr/local/lib -lboost_system -lpqxx -lssl -lcrypto -lpthread -lrt

RMALL = $(RM) *.o *.d


.DEFAULT_GOAL := all

%.o: %.cpp
	$(COMPILE.cpp) $(COMPILE_OPTIONS) $(OUTPUT_OPTION) $<

%:
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) $(LINK_OPTIONS) $(OUTPUT_OPTION)


ifneq ($(TRACK_DEPENDENCIES), 0)

TRACK_SYSTEM_DEPENDENCIES := 0
ifeq ($(TRACK_SYSTEM_DEPENDENCIES), 0)
override COMPILE_OPTIONS += -MMD
else
override COMPILE_OPTIONS += -MD
endif

override COMPILE_OPTIONS += -MP

%.d: ;

.PHONY: %.d

-include *.d ../common/*.d

endif
