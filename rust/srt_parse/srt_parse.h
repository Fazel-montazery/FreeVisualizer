#pragma once

#include <stdint.h>

typedef struct SrtTimeStamp {
	uint32_t h;
	uint32_t m;
	uint32_t s;
	uint32_t ms;
} SrtTimeStamp;


typedef struct SrtTimePeriod {
	SrtTimeStamp start;
	SrtTimeStamp end;
}

void process_srt(const char* path);
