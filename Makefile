TEST_DIR = test
INCLUDE_DIR = inc
BUILD_DIR = build
SRC_DIR = c
CC_FLAGS = -ggdb -I${INCLUDE_DIR} -lpthread -laio -luring
SOURCES = ${BUILD_DIR}/main.o ${BUILD_DIR}/common.o ${BUILD_DIR}/copy.o ${BUILD_DIR}/timer.o
INCLUDES = ${INCLUDE_DIR}/common.h ${INCLUDE_DIR}/config.h ${INCLUDE_DIR}/error_codes.h ${INCLUDE_DIR}/copy.h ${INCLUDE_DIR}/timer.h

${BUILD_DIR}/main.o: main.c ${BUILD_DIR} ${INCLUDES}
	gcc -o $@ -c main.c ${CC_FLAGS} 

${BUILD_DIR}/common.o: ${SRC_DIR}/common.c ${BUILD_DIR} ${INCLUDES}
	gcc -o $@ -c ${SRC_DIR}/common.c ${CC_FLAGS} 

${BUILD_DIR}/copy.o: ${SRC_DIR}/copy.c ${BUILD_DIR} ${INCLUDES}
	gcc -o $@ -c ${SRC_DIR}/copy.c ${CC_FLAGS}

${BUILD_DIR}/timer.o: ${SRC_DIR}/timer.c ${BUILD_DIR} ${INCLUDES}
	gcc -o $@ -c ${SRC_DIR}/timer.c ${CC_FLAGS}

fcp: ${SOURCES}
	gcc -o fcp ${SOURCES} ${CC_FLAGS}

all: fcp

${BUILD_DIR}:
	mkdir ${BUILD_DIR}

clean:
	rm -rf ${BUILD_DIR}
	rm -rf ${TEST_DIR}/*

install:
	cp ./fcp /usr/bin/fcp

.PHONY: all clean install
