.PHONY: clean

src := src/main.c
cc := gcc
target := fv
flags-release := -std=c99 -Wall -O2
flags-debug := -std=c99 -Wall -O0 -g3 -fsanitize=address

all: main

main: ${src}
	${cc} -o ${target} ${src} ${flags-release}

debug: ${src}
	${cc} -o ${target} ${src} ${flags-debug}

clean:
	rm ${target}
