CC:= g++
CFLAGS := -c -Wall -std=c++14 -g
INCLUDES:= -I. 
LDFLAGS:= -ltbb  -lboost_system


ALL_SOURCES := $(wildcard ./*.cpp)


OBJECTS:= $(ALL_SOURCES:.cpp=.o)
OBJECTS:= $(OBJECTS:.cc=.o)

EXECUTABLE= out

.PHONY: all test clean

all: $(ALL_SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
		$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@
		    
.cc.o:
		$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
		\rm -rf  $(OBJECTS) $(EXECUTABLE)
			    
