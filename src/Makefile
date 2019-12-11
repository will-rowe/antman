CC=				gcc-9
CFLAGS=			-g -Wall -O2 -Wc++-compat -I/usr/local/include
OBJS=			bloom.o config.o daemonize.o frozen.o hashmap.o heap.o murmurhash2.o sequence.o sketch.o slog.o watcher.o workerpool.o
LDFLAGS=		-L/usr/local/lib -L.
LIBS=			-lpthread -lm -lz -lfswatch -lantman
PROG=			antman
BIN=			../bin

.PHONY:all clean
.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $< -o $@

all:$(PROG)

antman:main.o libantman.a
		mkdir $(BIN)
		$(CC) $(CFLAGS) $(LDFLAGS) main.o -o $(BIN)/$@ $(LIBS)
		@echo "Donzo."

libantman.a:$(OBJS)
		$(AR) -csru $@ $(OBJS)

clean:
		rm -r *.o *.a $(BIN)

main.o: config.h daemonize.h ketopt.h helpers.h slog.h
bloom.o: bloom.h murmurhash2.h
config.o: config.h frozen.h
daemonize.o: daemonize.h bloom.h sequence.h slog.h watcher.h workerpool.h
hashmap.o: hashmap.h
heap.o: heap.h slog.h
murmurhash2.o: murmurhash2.h
sequence.o: sequence.h kseq.h sketch.h slog.h watcher.h
sketch.o: bloom.h hashmap.h heap.h slog.h
slog.o: slog.h
watcher.o: watcher.h sequence.h slog.h
workerpool.o: workerpool.h slog.h