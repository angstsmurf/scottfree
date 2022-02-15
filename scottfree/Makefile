GLK=glkterm
GLKINCLUDEDIR = ../$(GLK)
GLKLIBDIR = ../$(GLK)
GLKMAKEFILE = Make.$(GLK)
include $(GLKINCLUDEDIR)/$(GLKMAKEFILE)

CC = gcc
CFLAGS = -O2 -Wall -pedantic -ggdb -g3 -I$(GLKINCLUDEDIR)
LIBS = -L$(GLKLIBDIR) $(GLKLIB) $(LINKLIBS)

OBJS = bsd.o detectgame.o gameinfo.o layouttext.o load_TI99_4a.o parser.o restorestate.o scott.o

scottfree: $(OBJS)
	$(CC) -o scottfree $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) scottfree
