AUTOMAKE_OPTIONS =      foreign

CC=                     gcc
CFLAGS=                 -g -Wall -O2 -Wc++-compat -I/usr/local/include
OBJS=                   bloom.o config.o daemonize.o frozen.o hashmap.o heap.o murmurhash2.o sequence.o sketch.o slog.o watcher.o workerpool.o


%.o : %.c
		$(CC) -c $(CFLAGS) $< -o $@

libantman.a:$(OBJS)
		$(AR) -csru $@ $(OBJS)



bin_PROGRAMS = antman
antman_SOURCES = main.c config.h daemonize.h ketopt.h helpers.h slog.h
antman_CFLAGS = -g -Wall -O2 -Wc++-compat -I/usr/local/include
antman_LDADD = libantman.a -lpthread -lm -lz -lfswatch
antman_LDFLAGS = -L/usr/local/lib -L.


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