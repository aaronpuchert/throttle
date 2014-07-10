# Settings
CXX ?= clang++
CXXFLAGS = -Wall -std=c++11 -O3
LFLAGS = -Wall $(LIBOPTIONS)

# Files and libraries
SOURCES := main.cpp throttle.cpp conf.cpp
CPPS := $(patsubst %,src/%,$(SOURCES))
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

$(OBJS): %.o: %.cpp src/throttle.hpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

# Configuration
throttle.conf:
	touch throttle.conf
	./config >throttle.conf

# Debug
debug: debug/throttle debug/throttle.conf debug/pipe

debug/throttle: $(DEBUG_OBJS)
	-mkdir debug
	$(CXX) -g $(LFLAGS) -o $@ $(DEBUG_OBJS)

$(DEBUG_OBJS): %-debug.o: %.cpp src/throttle.hpp
	$(CXX) -c -g -DDEBUG $(CXXFLAGS) -o $@ $<

debug/throttle.conf:
	touch debug/throttle.conf
	(cd debug; ../config $(DEBUG_CORES) >throttle.conf)

debug/pipe:
	mkfifo debug/pipe

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
	-rm src/*.o throttle debug/throttle
	-rm debug/cpu*freq debug/temp_input debug/pipe
	-rm throttle.conf debug/throttle.conf

.PHONY: debug install uninstall clean
