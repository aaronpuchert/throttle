# Settings
CXX ?= clang++
CXXFLAGS = -Wall -std=c++11 -O3
LFLAGS = -Wall $(LIBOPTIONS)

# Files and libraries
CPPS := main.cpp throttle.cpp conf.cpp
OBJS := $(patsubst %.cpp, %.o, $(CPPS))
DEBUG_OBJS := $(patsubst %.cpp, %-debug.o, $(CPPS))

LIBS := pthread
LIBOPTIONS := $(patsubst %, -l%, $(LIBS))

# directory for service file
GENSYSTEMD := /lib/systemd/system
SYSTEMD := $(shell test -d /usr$(GENSYSTEMD) && echo /usr$(GENSYSTEMD))
ifeq ($(SYSTEMD),)
  SYSTEMD := $(shell test -d $(GENSYSTEMD) && echo $(GENSYSTEMD))
endif

DEBUG_CORES := 2

# Compiling
throttle: $(OBJS) throttle.conf
	$(CXX) $(LFLAGS) -o throttle $(OBJS)

$(OBJS): %.o: %.cpp throttle.hpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# Configuration
throttle.conf:
	touch throttle.conf
	./config >throttle.conf

# Debug
debug: throttle-debug throttle-debug.conf pipe

throttle-debug: $(DEBUG_OBJS)
	$(CXX) -g $(LFLAGS) -o throttle-debug $(DEBUG_OBJS)

$(DEBUG_OBJS): %-debug.o: %.cpp throttle.hpp
	$(CXX) -c -g -DDEBUG $(CXXFLAGS) -o $@ $<

throttle-debug.conf:
	touch throttle-debug.conf
	./config $(DEBUG_CORES) >throttle-debug.conf

pipe:
	mkfifo pipe

# Installation
install: throttle
	@install --verbose --strip --owner=root throttle /usr/sbin
	@install --verbose --owner=root throttle.conf /etc
	@install --verbose --owner=root throttle.service $(SYSTEMD)

uninstall:
	rm -f /usr/sbin/throttle
	rm -f /etc/throttle.conf
	rm -f $(SYSTEMD)/throttle.service

# Cleaning up
clean:
	-rm *.o throttle throttle-debug
	-rm cpu*freq temp_input pipe
	-rm throttle.conf throttle-debug.conf

.PHONY: debug install uninstall clean
