TESTS = $(check_PROGRAMS)
check_PROGRAMS = 	test_config \
                    test_heap \
                    test_bitvector \
                    test_bloomfilter \
                    test_bigsi \
                    test_nthash \
                    test_sequence

AM_CPPFLAGS =       -I${srcdir}/..
AM_CFLAGS =         -Wall -std=gnu99
LD_ADD =            ../libantman.a -lm

test_config_CFLAGS =              -std=gnu99 -g $(AM_CFLAGS)
test_config_LDADD =               $(LD_ADD)
test_heap_CFLAGS =                -std=gnu99 -g $(AM_CFLAGS)
test_heap_LDADD =                 $(LD_ADD)
test_bitvector_CFLAGS =           -std=gnu99 -g $(AM_CFLAGS)
test_bitvector_LDADD =            $(LD_ADD)
test_bloomfilter_CFLAGS =           -std=gnu99 -g $(AM_CFLAGS)
test_bloomfilter_LDADD =            $(LD_ADD)
test_bigsi_CFLAGS =               -std=gnu99 -g $(AM_CFLAGS)
test_bigsi_LDADD =                $(LD_ADD)
test_nthash_CFLAGS =               -std=gnu99 -g $(AM_CFLAGS)
test_nthash_LDADD =                $(LD_ADD)
test_sequence_CFLAGS =               -std=gnu99 -g $(AM_CFLAGS)
test_sequence_LDADD =                $(LD_ADD)