CC			= gcc -std=c99
# flags passati al compilatore per debugging e warning e altro
CFLAGS		= 	-g -Wall -pedantic -Wextra \
				-Wformat=2 -Wno-unused-parameter -Wshadow \
				-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
				-Wredundant-decls -Wnested-externs -Wmissing-include-dirs
INCLUDES 	= -I.
LDFLAGS 	= -L.
# librerie da linkare
LIBS    	= -lpthread

SRCDIR  	= src
OBJDIR   	= obj
BINDIR   	= bin
OBJS		= 	$(OBJDIR)/main.o			\
				$(OBJDIR)/utils.o			\
				$(OBJDIR)/config.o

TARGETS		= 	$(BINDIR)/supermercato \
				$(BINDIR)/direttore

TEST2CONFIGFILE = configtest2.txt

.PHONY: all clean cleanall test2

all: $(TARGETS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/supermercato: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BINDIR)/direttore:

clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f $(OBJDIR)/*.o *~ *.a
