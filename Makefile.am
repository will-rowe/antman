ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS =      foreign
SUBDIRS=                unit-tests
CLEANFILES =            libantman.a src/*.o src/3rd-party/*.o cmd/*.o
EXTRA_FLAGS =           -std=gnu99 -Wall -O2 -ggdb3 
LD_ADD =                -lpthread -lm -lz -ldb -lfswatch
OBJS =                  src/bitvector.o src/bloomfilter.o src/bigsi.o src/config.o src/daemonize.o src/hashmap.o src/heap.o src/helpers.o src/sequence.o src/sketch.o src/watcher.o src/workerpool.o src/3rd-party/frozen.o src/3rd-party/map.o src/3rd-party/murmurhash3.o src/3rd-party/slog.o

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
antman_SOURCES = cmd/main.c cmd/daemon.c cmd/info.c cmd/sketch.c cmd/subcommands.h src/config.h src/helpers.h src/3rd-party/ketopt.h
antman_LDADD = libantman.a $(LD_ADD)

# packages
bitvector.o: src/bitvector.h
bloomfilter.o: src/bloomfilter.h src/bitvector.h src/3rd-party/murmurhash3.h
bigsi.o: src/bigsi.h src/bitvector.h src/bloomfilter.h src/3rd-party/frozen.h src/3rd-party/map.h
config.o: src/config.h src/bloomfilter.h src/helpers.h src/3rd-party/frozen.h src/3rd-party/slog.h
daemonize.o: src/daemonize.h src/bloomfilter.h src/sequence.h src/3rd-party/slog.h src/watcher.h src/workerpool.h
hashmap.o: src/hashmap.h
heap.o: src/heap.h src/3rd-party/slog.h
helpers.o: src/helpers.h
map.o: src/3rd-party/map.h
murmurhash3.o: src/3rd-party/murmurhash3.h
sequence.o: src/sequence.h src/3rd-party/kseq.h src/sketch.h src/3rd-party/slog.h src/watcher.h
sketch.o: src/bloomfilter.h src/hashmap.h src/heap.h src/3rd-party/slog.h
slog.o: src/3rd-party/slog.h
watcher.o: src/watcher.h src/sequence.h src/3rd-party/slog.h
workerpool.o: src/workerpool.h src/3rd-party/slog.h