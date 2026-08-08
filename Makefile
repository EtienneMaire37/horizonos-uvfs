.PHONY: all clean
all: bin/uvfs-test
clean:
	rm -rf bin

export CC := gcc

bin/uvfs-test: src/vnode.c src/explore.c src/spinlock.h src/log.h src/explore.h src/vnode.h src/flags.h src/open_file.c src/open_file.h src/util/string.h src/util/string.c src/main.c src/user/posix_interface.h src/user/posix_interface.c Makefile
	mkdir -p bin
	${CC} -fanalyzer -fsanitize=undefined,address,leak -g -Og -o $@ src/vnode.c src/explore.c src/open_file.c src/util/string.c src/user/posix_interface.c src/main.c
