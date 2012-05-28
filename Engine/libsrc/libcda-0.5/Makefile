CC = gcc
CFLAGS = -Wall -O3
AR = ar

ifdef DJDIR
	# djgpp.
	OBJS = djgpp.o
	EXE = .exe
	LIBS =
else
ifdef windir			# (ugh)
	# mingw32.
	OBJS = windows.o
	EXE = .exe
	LIBS = -lwinmm
else
	# Assume Linux.
	OBJS = linux.o
	EXE = 
	LIBS =
endif
endif

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
