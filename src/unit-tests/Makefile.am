TESTS = $(check_PROGRAMS)
check_PROGRAMS = 	test_config \
                    test_heap \
                    test_sketch

AM_CPPFLAGS =       -I${srcdir}/..
AM_CFLAGS =         -Wall -std=gnu99
LD_ADD =            ../libantman.a -lm

test_config_CFLAGS=              -std=gnu99 -g $(AM_CFLAGS)
test_config_LDADD=               $(LD_ADD)
test_heap_CFLAGS=              -std=gnu99 -g $(AM_CFLAGS)
test_heap_LDADD=               $(LD_ADD)
test_sketch_CFLAGS=              -std=gnu99 -g $(AM_CFLAGS)
test_sketch_LDADD=               $(LD_ADD)