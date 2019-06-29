CPP=g++
CPPFLAGS=-fPIC -fvisibility-inlines-hidden -Wall -std=gnu++11
DEPS=$(wildcard *.h)

%.o: %.cpp $(DEPS)
	$(CPP) -c -o $@ $< $(CPPFLAGS)

libagspalrender: ags_palrender.o Raycast.o
	$(CPP) -shared -o libagspalrender.so ags_palrender.o Raycast.o $(CPPFLAGS)

.PHONY: clean
clean:
	rm -f *.gch *.o *.so
