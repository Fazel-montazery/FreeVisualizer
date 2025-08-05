#pragma once

#include <stdint.h>

typedef struct SrtTimeStamp {
	const uint32_t h;
	const uint32_t m;
	const uint32_t s;
	const uint32_t ms;
} SrtTimeStamp;

typedef struct SrtTimePeriod {
	const SrtTimeStamp start;
	const SrtTimeStamp end;
} SrtTimePeriod;

typedef struct SrtSection {
	const uint32_t		num;
	const SrtTimePeriod	period;
	const uintptr_t		str_start; // In the str_pool
	const uintptr_t		str_len;
} SrtSection;

typedef struct SrtHandle {
	const uintptr_t		sections_len;
	const SrtSection*	sections;
	const uintptr_t		str_pool_len;
	const uint8_t*		str_pool;
} SrtHandle;

SrtHandle process_srt(const char* path);
void free_srt(SrtHandle);
