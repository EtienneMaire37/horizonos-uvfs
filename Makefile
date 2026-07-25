.PHONY: all clean
all: bin/uvfs-test
clean:
	rm -rf bin

export CC := gcc

bin/libuvfs.a: src/vnode.c src/explore.c src/spinlock.h src/log.h src/explore.h src/vnode.h src/flags.h
	mkdir -p bin
	${CC} -fPIC -o bin/vnode.o -c src/vnode.c
	${CC} -fPIC -o bin/explore.o -c src/explore.c
	${CC} -shared -o $@ bin/explore.o bin/vnode.o

bin/uvfs-test: bin/libuvfs.a src/main.c
	mkdir -p bin
	${CC} src/main.c -o $@ -Lbin -luvfs
