
BUILD_DIR = build
CC_FLAGS = -ggdb
SOURCES = ${BUILD_DIR}/main.o

${BUILD_DIR}/main.o: main.c ${BUILD_DIR}
	gcc ${CC_FLAGS} -o $@ -c main.c

fcp: ${SOURCES}
	gcc ${CC_FLAGS} -o fcp ${SOURCES}

all: fcp

${BUILD_DIR}:
	mkdir ${BUILD_DIR}

.PHONY: all mkdir_build
