all:
		cd external/fswatch-1.14.0 && ./autogen.sh && ./configure && cd ../..
		$(MAKE) -C external/fswatch-1.14.0
		$(MAKE) -C external/fswatch-1.14.0 install
		$(MAKE) -C src

clean:
		$(MAKE) -C external/fswatch-1.14.0 distclean
		$(MAKE) -C src clean

test:
		$(MAKE) -C tests