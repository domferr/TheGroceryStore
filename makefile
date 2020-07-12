CC			= gcc -std=c99
# flags passati al compilatore per debugging e warning e altro
CFLAGS		= -g -Wall -pedantic -Wextra \
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
			 	$(OBJDIR)/grocerystore.o	\
				$(OBJDIR)/config.o			\
				$(OBJDIR)/scfiles.o			\
				$(OBJDIR)/threadpool.o		\
				$(OBJDIR)/queue.o			\
				$(OBJDIR)/client.o			\
				$(OBJDIR)/cashier.o			\
				$(OBJDIR)/signal_handler.o	\
				$(OBJDIR)/utils.o			\

TARGETS		= 	$(BINDIR)/grocerystore

.PHONY: all clean cleanall

all: $(TARGETS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/grocerystore: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

clean		:
	rm -f $(TARGETS)

cleanall	: clean
	\rm -f $(OBJDIR)/*.o *~ *.a
