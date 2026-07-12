.PHONY: install uninstall clean

INSDIR ?= /usr/local
BINDIR = ${INSDIR}/bin
USR_HOME ?= $(shell getent passwd $(SUDO_USER) | cut -d: -f6)
DATADIR ?= ${USR_HOME}/.FreeVisualizer
SHADERDIR := ${DATADIR}/shaders
SHADERS := shaders/*

SRC := src/main.c src/opts.c src/fs.c include/glad/glad.c src/shader.c src/srt_parse.c
LIBS := -lglfw -lpthread -lsndfile rust/srt_parse/target/release/libsrt_parse.a $(shell pkg-config --cflags --libs libpipewire-0.3)
CC := gcc
TARGET := fv
FLAGS_RELEASE := -Wall -O2 -DNDEBUG
FLAGS_DEBUG := -Wall -O0 -g3 -fsanitize=address

BASH_COMPLETION_DIR := $(shell pkg-config --variable=completionsdir bash-completion 2>/dev/null || echo /etc/bash_completion.d)
ZSH_COMPLETION_DIR  := /usr/local/share/zsh/site-functions
FISH_COMPLETION_DIR := $(shell pkg-config --variable=completionsdir fish 2>/dev/null || echo $(PREFIX)/share/fish/vendor_completions.d)

all: main

main: ${SRC}
	cargo build --release --manifest-path ./rust/srt_parse/Cargo.toml
	${CC} -o ${TARGET} ${SRC} ${LIBS} ${FLAGS_RELEASE}

debug: ${SRC}
	cargo build --release --manifest-path ./rust/srt_parse/Cargo.toml
	${CC} -o ${TARGET} ${SRC} ${LIBS} ${FLAGS_DEBUG}

install: ${TARGET}
	strip ${TARGET}
	install -d ${BINDIR}
	install -m 755 ${TARGET} ${BINDIR}/${TARGET}
	rm -rf ${DATADIR}
	install -d -m 777 ${DATADIR}
	install -m 666 colors.txt ${DATADIR}/
	rm -rf ${SHADERDIR}
	install -d ${SHADERDIR}
	install -m 444  ${SHADERS} ${SHADERDIR}/
	@if command -v bash >/dev/null 2>&1; then \
		install -Dm644 autocomplete/fv.bash $(DESTDIR)$(BASH_COMPLETION_DIR)/fv; \
		echo "  bash -> $(BASH_COMPLETION_DIR)/fv"; \
	fi
	@if command -v zsh >/dev/null 2>&1; then \
		install -Dm644 autocomplete/_fv $(DESTDIR)$(ZSH_COMPLETION_DIR)/_fv; \
		echo "  zsh  -> $(ZSH_COMPLETION_DIR)/_fv"; \
	fi
	@if command -v fish >/dev/null 2>&1; then \
		install -Dm644 autocomplete/fv.fish $(DESTDIR)$(FISH_COMPLETION_DIR)/fv.fish; \
		echo "  fish -> $(FISH_COMPLETION_DIR)/fv.fish"; \
	fi

uninstall:
	rm -f ${BINDIR}/${TARGET}
	rm -rf ${DATADIR}

clean:
	rm ${TARGET}
