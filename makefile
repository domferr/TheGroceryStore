# compilatore
CC			= gcc -std=c99
# flags passati al compilatore per debugging e warning e altro
CFLAGS		= -g -Wall -pedantic -Wextra \
                                           -Wformat=2 -Wno-unused-parameter -Wshadow \
                                           -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
                                           -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
INCLUDES 	= -I.
LDFLAGS 	= -L.
# librerie da linkare
LIBS    	= -pthread

TARGETS		= grocerystore
SRCDIR  	= src
OBJDIR   	= obj
BINDIR   	= bin

.PHONY: all

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGETS)

grocerystore: $(OBJDIR)/grocerystore.o $(OBJDIR)/config.o $(OBJDIR)/scfiles.o $(OBJDIR)/threadpool.o $(OBJDIR)/queue.o $(OBJDIR)/client.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $(BINDIR)/$@ $^ $(LIBS)
