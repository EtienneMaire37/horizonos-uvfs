.PHONY: all clean
all: bin/uvfs-test
clean:
	rm -rf bin

export CC := gcc

bin/libuvfs.a: bin/explore.o bin/vnode.o
	${CC} -shared -o $@ bin/explore.o bin/vnode.o -ltsan -lubsan

bin/vnode.o: src/vnode.c src/spinlock.h src/log.h src/explore.h src/vnode.h src/flags.h Makefile
	mkdir -p bin
	${CC} -fPIC -fanalyzer -fsanitize=undefined,thread -g -o bin/vnode.o -c src/vnode.c -ltsan -lubsan

bin/explore.o: src/explore.c src/spinlock.h src/log.h src/explore.h src/vnode.h src/flags.h Makefile
	mkdir -p bin
	${CC} -fPIC -fanalyzer -fsanitize=undefined,thread -g -o bin/explore.o -c src/explore.c -ltsan -lubsan

bin/uvfs-test: bin/libuvfs.a src/main.c
	mkdir -p bin
	${CC} src/main.c -o $@ -Lbin -luvfs
