CC = gcc
CPP = g++
PRODUCT = zlib-test
FLAGS = -Wall -Wno-exit-time-destructors -Wno-global-constructors -Wno-padded -Wno-old-style-cast
CWD = $(shell pwd)
SRC = ${CWD}/src
BUILD = ${CWD}/build
ZLIB_ARC = ${SRC}/zlib-1.2.8.tar.gz
ZLIB_DIR = ${SRC}/zlib-1.2.8
ZLIB_SYMDIR = ${SRC}/zlib
ZLIB_LIBDIR = ${ZLIB_SYMDIR}
FLAGS2 = -O3 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDEBUG
INC = ${SRC}
UNAME := $(shell uname -s)

ifeq ($(UNAME),Darwin)
	CC = clang
	CPP = clang++
	FLAGS += -Weverything
endif

all: prep zlib
	${CPP} ${FLAGS} ${FLAGS2} -I${INC} -c "${SRC}/${PRODUCT}.cpp" -o "${BUILD}/${PRODUCT}.o"
	${CPP} ${FLAGS} ${FLAGS2} -I${INC} -I${ZLIB_SYMDIR} -L"${ZLIB_LIBDIR}" "${BUILD}/${PRODUCT}.o" -o "${BUILD}/${PRODUCT}" -lz

prep:
	if [ ! -d "${BUILD}" ]; then mkdir "${BUILD}"; fi

zlib:
	if [ ! -d "${ZLIB_DIR}" ]; then mkdir "${ZLIB_DIR}"; fi
	tar zxvf "${ZLIB_ARC}" -C "${SRC}"
	ln -sf ${ZLIB_DIR} ${ZLIB_SYMDIR}
	cd ${ZLIB_SYMDIR} && ./configure --static && cd ${CWD}
	${MAKE} -C ${ZLIB_SYMDIR} libz.a CC=${CC} 

clean:
	rm -rf *~
	rm -rf ${SRC}/*~
	rm -rf ${ZLIB_DIR}
	rm -rf ${ZLIB_SYMDIR}
	rm -rf ${BUILD}
	rm -rf *.zlib-test
	rm -rf *.zlib-test.out
