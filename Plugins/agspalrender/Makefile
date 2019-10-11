INCDIR = ../../Engine ../../Common
CXX=g++
CXXFLAGS=-fPIC -fvisibility-inlines-hidden -Wall -std=gnu++11 $(addprefix -I,$(INCDIR))
DEPS=$(wildcard *.h)

all: libagspalrender.so

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

libagspalrender.so: ags_palrender.o raycast.o
	$(CXX) -shared -o libagspalrender.so ags_palrender.o raycast.o $(CXXFLAGS)

clean:
	rm -f *.gch *.o *.so

.PHONY: clean
