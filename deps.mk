override CXXFLAGS += -MMD

%.d:

.PHONY: %.d

-include *.d ../common/*.d
