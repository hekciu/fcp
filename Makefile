
INCLUDE_DIR = inc
BUILD_DIR = build
SRC_DIR = c
CC_FLAGS = -ggdb -I${INCLUDE_DIR}
SOURCES = ${BUILD_DIR}/main.o ${BUILD_DIR}/common.o
INCLUDES = ${INCLUDE_DIR}/common.h ${INCLUDE_DIR}/config.h ${INCLUDE_DIR}/error_codes.h

${BUILD_DIR}/main.o: main.c ${BUILD_DIR} ${INCLUDES}
	gcc ${CC_FLAGS} -o $@ -c main.c

${BUILD_DIR}/common.o: ${SRC_DIR}/common.c ${BUILD_DIR} ${INCLUDES}
	gcc ${CC_FLAGS} -o $@ -c ${SRC_DIR}/common.c

fcp: ${SOURCES}
	gcc ${CC_FLAGS} -o fcp ${SOURCES}

all: fcp

${BUILD_DIR}:
	mkdir ${BUILD_DIR}

.PHONY: all mkdir_build
