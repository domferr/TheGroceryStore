# compilatore
CC			= gcc -std=c99
AR      	= ar
# flags passati al compilatore per debugging e warning e altro
CFLAGS		= -g -Wall -pedantic -Wextra \
                                           -Wformat=2 -Wno-unused-parameter -Wshadow \
                                           -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
                                           -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
ARFLAGS 	= rvs
INCLUDES 	= -I.
LDFLAGS 	= -L.
# librerie da linkare
LIBS    	= -pthread

TARGETS		= grocerystore
SRCDIR  	= src
OBJDIR   	= obj
BINDIR   	= bin
LIBDIR   	= lib

.PHONY: all

# Per ogni .o ho bisogno del suo .c e compilo. Mi fermo prima di chiamare il linker
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGETS)

grocerystore: $(OBJDIR)/grocerystore.o $(OBJDIR)/config.o $(OBJDIR)/scfiles.o $(OBJDIR)/executor.o $(OBJDIR)/executorspool.o $(OBJDIR)/queue.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $(BINDIR)/$@ $^ $(LIBS)
