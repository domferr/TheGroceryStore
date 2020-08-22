CC		= gcc -std=c99
CFLAGS	= -g -Wall -pedantic -Wextra \
			-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
			-Wformat=2 -Wno-unused-parameter -Wshadow \
			-Wredundant-decls -Wnested-externs -Wmissing-include-dirs

SRCDIR  	= ./src
INCDIR		= ./include
OBJDIR   	= ./obj
BINDIR   	= ./bin
LIBDIR      = ./libs

INCLUDES 	= -I $(INCDIR)
LDFLAGS 	= -L $(LIBDIR)
#librerie usate dal processo direttore
LIBSDIRETT  = -lpthread -lutils
#librerie usate dal processo supermercato
LIBSSUPERM  = -lpthread -llist -lutils

#dipendenze in comune
OBJS_SHARED = 	$(OBJDIR)/config.o			\
             	$(OBJDIR)/scfiles.o			\
             	$(OBJDIR)/af_unix_conn.o	\
              	$(OBJDIR)/sig_handling.o	\
              	$(LIBDIR)/libutils.so
#dipendenze per l'eseguibile del supermercato
OBJS_SUPERM	= 	$(OBJDIR)/supermercato.o	\
				$(OBJDIR)/threadpool.o		\
				$(OBJDIR)/store.o			\
				$(OBJDIR)/client.o			\
				$(OBJDIR)/cassiere.o		\
				$(OBJDIR)/client_in_queue.o	\
				$(OBJDIR)/notifier.o		\
				$(OBJDIR)/cassa_queue.o		\
				$(OBJDIR)/log.o				\
				$(LIBDIR)/liblist.so		\
				$(OBJS_SHARED)
# dipendenze per l'eseguibile del direttore
OBJS_DIRETT	=	$(OBJDIR)/direttore.o		\
             	$(OBJS_SHARED)

TARGETS	= $(BINDIR)/direttore $(BINDIR)/supermercato

CONFIGTEST1FILE = configtest1.txt
CONFIGTEST2FILE = configtest2.txt
LOGFILE = testlog.csv

.PHONY: all clean cleanall test1 test2 $(CONFIGTEST1FILE) $(CONFIGTEST2FILE)

all: $(BINDIR) $(OBJDIR) $(TARGETS)

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

# generazione di un .o da un .c con il relativo .h come dipendenza
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# generazione di un .o da un .c senza relativo .h
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# da .c ad eseguibile del supermercato
$(BINDIR)/supermercato: $(OBJS_SUPERM)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS) $(LIBSSUPERM)

# da .c ad eseguibile del direttore
$(BINDIR)/direttore: $(OBJS_DIRETT)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBSDIRETT)

$(LIBDIR)/list/list.o: $(LIBDIR)/list/src/list.c $(LIBDIR)/list/include/list.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fPIC -o $@ $<

$(LIBDIR)/utils/utils.o: $(LIBDIR)/utils/src/utils.c $(LIBDIR)/utils/include/utils.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -fPIC -o $@ $<

$(LIBDIR)/liblist.so: $(LIBDIR)/list/list.o
	$(CC) -shared -o $@ $<

$(LIBDIR)/libutils.so: $(LIBDIR)/utils/utils.o
	$(CC) -shared -o $@ $<

# scrittura del file di configurazione per il test1
$(CONFIGTEST1FILE):
	@echo "K = 2"  		>$@
	@echo "C = 20" 		>>$@
	@echo "E = 5"  		>>$@
	@echo "T = 500"  	>>$@
	@echo "P = 80"  	>>$@
	@echo "S = 30"  	>>$@
	@echo "KT = 2"  	>>$@
	@echo "KA = 2"  	>>$@
	@echo "S1 = 1"  	>>$@
	@echo "S2 = 4"  	>>$@
	@echo "D = 500"  	>>$@
	@echo "L = $(LOGFILE)" >>$@

# scrittura del file di configurazione per il test2
$(CONFIGTEST2FILE):
	@echo "K = 6"  		>$@
	@echo "C = 50" 		>>$@
	@echo "E = 3"  		>>$@
	@echo "T = 200"  	>>$@
	@echo "P = 100"  	>>$@
	@echo "S = 20"  	>>$@
	@echo "KT = 3"  	>>$@
	@echo "KA = 5"  	>>$@
	@echo "S1 = 5"  	>>$@
	@echo "S2 = 3"  	>>$@
	@echo "D = 500"  	>>$@
	@echo "L = $(LOGFILE)" >>$@

# lancio del primo test. Necessario che il file di configurazione sia stato creato e che l'eseguibile sia stato generato
test1: $(CONFIGTEST1FILE) all
	@echo "Running Test 1"
	@valgrind --leak-check=full --trace-children=yes $(BINDIR)/direttore -c $< & sleep 15; kill -s 3 $$!; wait $$!

# lancio del secondo test. Necessario che il file di configurazione sia stato creato e che l'eseguibile sia stato generato
test2: $(CONFIGTEST2FILE) all
	@chmod +x ./analisi.sh;
	@echo "Running Test 2"
	@$(BINDIR)/direttore -c $< & sleep 25; kill -s 1 $$!; wait $$!
	@./analisi.sh $(LOGFILE)

clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f $(OBJDIR)/*.o $(LIBDIR)/*/*.o $(LIBDIR)/*.so *~ *.a *.sock $(CONFIGTEST1FILE) $(CONFIGTEST2FILE)
