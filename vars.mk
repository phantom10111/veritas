override CXXFLAGS += -std=gnu++11 -I$(ROOT)/include -O2
override LDLIBS += -lboost_system -lpqxx
RM := rm -rf
GENCLEAN := *.o
