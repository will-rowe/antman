AUTOMAKE_OPTIONS =      foreign
SUBDIRS=                unit-tests
CLEANFILES =            libantman.a
EXTRA_FLAGS =           -g -Wall -O2 -Wc++-compat
LD_ADD =                -lpthread -lm -lz -lfswatch
OBJS =                  bloom.o config.o daemonize.o frozen.o hashmap.o heap.o murmurhash2.o sequence.o sketch.o slog.o watcher.o workerpool.o
default_config =        "/tmp/.antman.config"
default_watch_dir =     "/var/lib/MinKNOW/data/reads"


%.o : %.c
		$(CC) -c $(CFLAGS) \
		-DAM_VERSION=\"@VERSION@\" \
		-DAM_DEFAULT_CONFIG=\"$(default_config)\" \
		-DAM_DEFAULT_WATCH_DIR=\"$(default_watch_dir)\" \
		$< -o $@

libantman.a:$(OBJS)
		$(AR) -csru $@ $(OBJS)

bin_PROGRAMS = antman
antman_SOURCES = main.c config.h daemonize.h ketopt.h slog.h
antman_LDADD = libantman.a $(LD_ADD)

main.o: config.h daemonize.h ketopt.h slog.h
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