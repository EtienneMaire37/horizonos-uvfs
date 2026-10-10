export CC := gcc

SOURCE_CU := $(shell find src -name '*.c')
SOURCE := $(shell find src -name '*.h') $(SOURCE_CU)

.PHONY: valdebug debug release all clean
all: release
clean:
	rm -rf bin

valdebug: $(SOURCE) Makefile
	mkdir -p bin
	${CC} -Og -g -march=native -Wall -Wextra -Wno-missing-field-initializers -Werror -o bin/uvfs-test $(SOURCE_CU) ${CFLAGS}
debug: $(SOURCE) Makefile
	mkdir -p bin
	${CC} -fanalyzer -fsanitize=address,undefined,leak -g -Og -march=native -Wall -Wextra -Wno-missing-field-initializers -Werror -o bin/uvfs-test $(SOURCE_CU) ${CFLAGS}
release: $(SOURCE) Makefile
	mkdir -p bin
	${CC} -O3 -DNDEBUG -march=native -Wall -Wextra -Wno-missing-field-initializers -o bin/uvfs-test $(SOURCE_CU) ${CFLAGS}
