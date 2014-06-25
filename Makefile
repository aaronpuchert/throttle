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
debug: throttle-debug throttle-debug.conf

throttle-debug: $(DEBUG_OBJS)
	$(CXX) -g $(LFLAGS) -o throttle-debug $(DEBUG_OBJS)

$(DEBUG_OBJS): %-debug.o: %.cpp throttle.hpp
	$(CXX) -c -g -DDEBUG $(CXXFLAGS) -o $@ $<

throttle-debug.conf:
	touch throttle-debug.conf
	./config debug >throttle-debug.conf

# Installation
install: throttle

# Cleaning up
clean:
	-rm *.o throttle
	-rm cpu*freq temp_input throttle-debug
	-rm throttle.conf throttle-debug.conf

.PHONY: debug install clean
