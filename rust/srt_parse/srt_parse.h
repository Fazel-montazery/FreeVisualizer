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

typedef struct SrtHandle {
	uintptr_t	sections_len;
	SrtSection*	sections;
} SrtHandle;

SrtHandle process_srt(const char* path);
void free_srt(SrtHandle);
