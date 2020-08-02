CC		= gcc -std=c99
CFLAGS	= -g -Wall -pedantic -Wextra \
			-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
			-Wformat=2 -Wno-unused-parameter -Wshadow \
			-Wredundant-decls -Wnested-externs -Wmissing-include-dirs

SRCDIR  	= ./src
INCDIR		= ./include
OBJDIR   	= ./obj
BINDIR   	= ./bin

INCLUDES 	= -I $(INCDIR)
LDFLAGS 	= -L.
LIBS    	= -lpthread

#dipendenze per l'eseguibile del supermercato
OBJS_SUPERM	= 	$(OBJDIR)/supermercato.o	\
				$(OBJDIR)/utils.o			\
				$(OBJDIR)/config.o			\
				$(OBJDIR)/scfiles.o			\
				$(OBJDIR)/queue.o			\
				$(OBJDIR)/threadpool.o		\
				$(OBJDIR)/af_unix_conn.o	\
				$(OBJDIR)/sig_handling.o	\
				$(OBJDIR)/store.o
# dipendenze per l'eseguibile del direttore
OBJS_DIRETT	=	$(OBJDIR)/direttore.o		\
             	$(OBJDIR)/utils.o			\
				$(OBJDIR)/config.o			\
				$(OBJDIR)/scfiles.o			\
				$(OBJDIR)/queue.o			\
				$(OBJDIR)/threadpool.o		\
				$(OBJDIR)/af_unix_conn.o	\
				$(OBJDIR)/sig_handling.o

TARGETS	= $(BINDIR)/direttore $(BINDIR)/supermercato

CONFIGFILE = configtest.txt
LOGFILE = testlog.csv

.PHONY: all clean cleanall test1 test2

all: $(TARGETS)

# generazione di un .o da un .c con il relativo .h come dipendenza
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# generazione di un .o da un .c senza relativo .h
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# da .c ad eseguibile del supermercato
$(BINDIR)/supermercato: $(OBJS_SUPERM)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(LIBS)

# da .c ad eseguibile del direttore
$(BINDIR)/direttore: $(OBJS_DIRETT)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)

test1: all
	@echo "Running test1"
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
	@valgrind --leak-check=full --trace-children=yes $(BINDIR)/direttore -c $(CONFIGFILE) & sleep 3; kill -s 3 $$!; wait $$!

test2: all
	@echo "Running test2"
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
	@$(BINDIR)/direttore -c $(CONFIGFILE) & sleep 5; kill -s 1 $$!; wait $$!
	@./analisi.sh

clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f $(OBJDIR)/*.o *~ *.a *.sock $(CONFIGFILE)
