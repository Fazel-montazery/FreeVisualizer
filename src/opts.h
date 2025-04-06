#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <getopt.h>

#include "defs.h"
#include "fs.h"

// returns false if exit condition, else true
// caller must allocate fragShaderPathBuf
// bufferSiz is the size of fragShaderPathBuf
// musicPath is just a pointer, not a buffer. You should pass a char** to it
bool parseOpts( int argc, 
		char *argv[],
		char** musicPath,
		char* fragShaderPathBuf,
		size_t bufferSiz,
		bool* fullscreen);
