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
OBJS		= 	$(OBJDIR)/scfiles.o			\
				$(OBJDIR)/queue.o			\
				$(OBJDIR)/threadpool.o		\
				$(OBJDIR)/config.o			\
			 	$(OBJDIR)/grocerystore.o	\
				$(OBJDIR)/cashier.o			\
				$(OBJDIR)/client.o			\
				$(OBJDIR)/utils.o			\
				$(OBJDIR)/client_in_queue.o	\
				$(OBJDIR)/signal_handler.o	\
				$(OBJDIR)/logger.o			\
				$(OBJDIR)/main.o			\
				$(OBJDIR)/manager.o			\

TARGETS		= 	$(BINDIR)/grocerystore

TEST2CONFIGFILE = configtest2.txt

.PHONY: all clean cleanall test2

all: $(TARGETS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/grocerystore: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f $(OBJDIR)/*.o *~ *.a

test2:
	@echo "K=6\nKA=5\nKT=35\nC=50\nE=3\nT=200\nP=100\nS=20\nS1=5\nS2=15\nD=1000\nL=logfile.txt" > $(TEST2CONFIGFILE)
	$(BINDIR)/grocerystore -c $(TEST2CONFIGFILE)
	sleep 25 ;\
    	kill -s HUP "$$!" ;\
    	wait $$!
