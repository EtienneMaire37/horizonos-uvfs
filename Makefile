.PHONY: all clean
all: bin/uvfs-test
clean:
	rm -rf bin

export CC := gcc

bin/uvfs-test: src/vnode.c src/explore.c src/spinlock.h src/log.h src/explore.h src/vnode.h src/flags.h src/main.c Makefile
	mkdir -p bin
	${CC} -fanalyzer -fsanitize=undefined,address,leak -g -Og -o $@ src/vnode.c src/explore.c src/main.c
