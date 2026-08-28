.PHONY: all clean
all: bin/uvfs-test
clean:
	rm -rf bin

export CC := gcc

SOURCE_CU := $(shell find src -name '*.c')
SOURCE := $(shell find src -name '*.h') $(SOURCE_CU)

bin/uvfs-test: $(SOURCE) Makefile
	mkdir -p bin
	${CC} -fanalyzer -fsanitize=undefined,address,leak -g -Og -march=native -Wall -Wextra -Werror -o $@ $(SOURCE_CU) ${CFLAGS}
