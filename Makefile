all:
		cd external/fswatch-1.14.0 && touch * && ./configure CC=c99 CFLAGS=-g && cd ../..
		$(MAKE) -C external/fswatch-1.14.0
		$(MAKE) -C external/fswatch-1.14.0 install
		$(MAKE) -C src

clean:
		$(MAKE) -C external/fswatch-1.14.0 distclean
		$(MAKE) -C src clean

test:
		$(MAKE) -C tests