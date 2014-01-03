CXXFLAGS += -O2
override CXXFLAGS += -std=gnu++11 -I$(ROOT)/include
override LDLIBS += -lboost_system -lpqxx

RM := rm -rf
GENCLEAN := *.o *.d
RMALL = $(RM) $(GENCLEAN)

.DEFAULT_GOAL := all
