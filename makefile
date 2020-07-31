CC			= gcc -std=c99
# flags passati al compilatore per debugging e warning e altro
CFLAGS		= 	-g -Wall -pedantic -Wextra \
				-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
				-Wformat=2 -Wno-unused-parameter -Wshadow \
				-Wredundant-decls -Wnested-externs -Wmissing-include-dirs
INCLUDES 	= -I.
LDFLAGS 	= -L.
# librerie da linkare
LIBS    	= -lpthread

SRCDIR  	= src
OBJDIR   	= obj
BINDIR   	= bin

OBJS_SUPERM	=

OBJS_DIRETT	=	$(OBJDIR)/direttore.o		\
             	$(OBJDIR)/utils.o			\
				$(OBJDIR)/config.o

TARGETS	= $(BINDIR)/direttore


CONFIGFILE = configtest.txt
LOGFILE = testlog.csv

.PHONY: all clean cleanall test1 test2

all: $(TARGETS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

#$(BINDIR)/supermercato: $(OBJS_SUPERM)
	#$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BINDIR)/direttore: $(OBJS_DIRETT)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

test1:
	@echo "K = 2"  		>$(CONFIGFILE)
	@echo "KT = 35"  	>>$(CONFIGFILE)
	@echo "KA = 1"  	>>$(CONFIGFILE)
	@echo "C = 20" 		>>$(CONFIGFILE)
	@echo "E = 5"  		>>$(CONFIGFILE)
	@echo "T = 500"  	>>$(CONFIGFILE)
	@echo "P = 80"  	>>$(CONFIGFILE)
	@echo "D = 1000"  	>>$(CONFIGFILE)
	@echo "S = 30"  	>>$(CONFIGFILE)
	@echo "S1 = 2"  	>>$(CONFIGFILE)
	@echo "S2 = 6"  	>>$(CONFIGFILE)
	@echo "L = $(LOGFILE)" >>$(CONFIGFILE)
	valgrind --leak-check=full --trace-children=yes ./$(BINDIR)/direttore -c $(CONFIGFILE) & sleep 15; \
	kill -s 3 $$!;

#wait $$!

test2:
	@echo "K = 6"  		>$(CONFIGFILE)
	@echo "KT = 35"  	>>$(CONFIGFILE)
	@echo "KA = 5"  	>>$(CONFIGFILE)
	@echo "C = 50" 		>>$(CONFIGFILE)
	@echo "E = 3"  		>>$(CONFIGFILE)
	@echo "T = 200"  	>>$(CONFIGFILE)
	@echo "P = 100"  	>>$(CONFIGFILE)
	@echo "S = 20"  	>>$(CONFIGFILE)
	@echo "S1 = 5"  	>>$(CONFIGFILE)
	@echo "S2 = 15"  	>>$(CONFIGFILE)
	@echo "D = 1000"  	>>$(CONFIGFILE)
	@echo "L = $(LOGFILE)" >>$(CONFIGFILE)
	./$(BINDIR)/direttore -c $(CONFIGFILE) & sleep 5; \
	kill -s 1 $$!;	\
	./analisi.sh

clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f $(OBJDIR)/*.o *~ *.a
