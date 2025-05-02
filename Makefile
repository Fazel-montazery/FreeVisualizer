.PHONY: install uninstall clean

INSDIR ?= /usr/local
BINDIR = ${INSDIR}/bin
USR_HOME ?= $(shell getent passwd $(SUDO_USER) | cut -d: -f6)
DATADIR ?= ${USR_HOME}/.FreeVisualizer
SHADERDIR := ${DATADIR}/shaders
SHADERS := shaders/*

SRC := src/main.c src/opts.c src/fs.c src/glad.c src/shader.c
LIBS := -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lasound -lsndfile
CC := gcc
TARGET := fv
FLAGS_RELEASE := -Wall -O2 -DNDEBUG
FLAGS_DEBUG := -Wall -O0 -g3 -fsanitize=address

all: main

main: ${SRC}
	${CC} -o ${TARGET} ${SRC} ${LIBS} ${FLAGS_RELEASE}

debug: ${SRC}
	${CC} -o ${TARGET} ${SRC} ${LIBS} ${FLAGS_DEBUG}

install: ${TARGET}
	install -d ${BINDIR}
	install -m 755 ${TARGET} ${BINDIR}/${TARGET}
	rm -rf ${SHADERDIR}
	install -d ${SHADERDIR}
	install -m 644  ${SHADERS} ${SHADERDIR}/

uninstall:
	rm -f ${BINDIR}/${TARGET}
	rm -rf ${DATADIR}

clean:
	rm ${TARGET}
