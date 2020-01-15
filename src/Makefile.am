AUTOMAKE_OPTIONS =      foreign
SUBDIRS=                unit-tests
CLEANFILES =            libantman.a
EXTRA_FLAGS =           -std=gnu99 -Wall -O2 -ggdb3 
LD_ADD =                -lpthread -lm -lz -lfswatch
OBJS =                  bigsi.o bloom.o config.o daemonize.o frozen.o hashmap.o heap.o map.o murmurhash2.o sequence.o sketch.o slog.o watcher.o workerpool.o

%.o : %.c
		$(CC) -c $(CFLAGS) $(EXTRA_FLAGS) \
		-DPROG_NAME=\"@PROG_NAME@\" \
		-DPROG_VERSION=\"@VERSION@\" \
		-DCONFIG_LOCATION=\"@CONFIG_LOCATION@\" \
		-DDEFAULT_WATCH_DIR=\"@DEFAULT_WATCH_DIR@\" \
		$< -o $@

libantman.a:$(OBJS)
		$(AR) -csru $@ $(OBJS)

bin_PROGRAMS = antman
antman_SOURCES = main.c bloom.h config.h daemonize.h ketopt.h sequence.h slog.h watcher.h
antman_LDADD = libantman.a $(LD_ADD)

bigsi.o: bigsi.h map.h
bloom.o: bloom.h murmurhash2.h
config.o: bloom.h config.h frozen.h slog.h
daemonize.o: daemonize.h bloom.h sequence.h slog.h watcher.h workerpool.h
hashmap.o: hashmap.h
heap.o: heap.h slog.h
map.o: map.h
murmurhash2.o: murmurhash2.h
sequence.o: sequence.h kseq.h sketch.h slog.h watcher.h
sketch.o: bloom.h hashmap.h heap.h slog.h
slog.o: slog.h
watcher.o: watcher.h sequence.h slog.h
workerpool.o: workerpool.h slog.h