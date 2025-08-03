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
} SrtTimePeriod;

typedef struct SrtSection {
	uint32_t	num;
	SrtTimePeriod	period;
} SrtSection;

SrtSection* process_srt(const char* path, uintptr_t* len_dst);
void free_srt(SrtSection* sections, uintptr_t len);
