# Makefile for BeOS / gcc.  
# Will put this into main Makefile once I figure out how to 
# autodetect BeOS.

CC = gcc
CFLAGS = -Wall -Wno-multichar -Wno-ctor-dtor-privacy

OBJS = beos.o
EXE = 
LIBS = -lbe

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $< 

LIBCDA = libcda.a
EXAMPLE = example$(EXE)

all: $(LIBCDA) $(EXAMPLE)

$(LIBCDA): $(OBJS)
	$(AR) rs $@ $^

$(EXAMPLE): example.o $(LIBCDA)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(LIBCDA) $(OBJS) 
	rm -f $(EXAMPLE) example.o
	rm -f *~	
