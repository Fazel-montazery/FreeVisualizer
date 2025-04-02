.PHONY: clean

src := src/main.c src/fs.c src/glad.c src/shader.c
libs := -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl
cc := gcc
target := fv
flags-release := -std=c99 -Wall -O2 -DNDEBUG
flags-debug := -std=c99 -Wall -O0 -g3 -fsanitize=address

all: main

main: ${src}
	${cc} -o ${target} ${src} ${libs} ${flags-release}

debug: ${src}
	${cc} -o ${target} ${src} ${libs} ${flags-debug}

clean:
	rm ${target}
