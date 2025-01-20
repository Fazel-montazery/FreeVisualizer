src := src/main.c
libs := -lSDL3 -lmpg123
flags := -std=c99 -Wall -O2
target := bin/FreeVisualizer.out
cc := gcc

all: main

main: ${src}
	${cc} -o ${target} ${src} ${libs} ${flags}

run:
	./${target}