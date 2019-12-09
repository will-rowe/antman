CC        ?= gcc
CFLAGS    = -Wall -Wno-unknown-pragmas -Werror -O0 -std=c99 -D DEBUG=1 -I /usr/include -lm

test:
	@echo "Running unit tests..."
	@echo " - compiling tests"
	@$(CC) -o _test_config config_test.c $(CFLAGS)
	@$(CC) -o _test_heap heap_test.c $(CFLAGS)
	@$(CC) -o _test_sketch sketch_test.c $(CFLAGS)
	@echo " - launching test runner"
	@./run_unit_tests.sh
	@echo " - cleaning up"
	@rm _test*