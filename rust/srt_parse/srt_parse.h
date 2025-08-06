#pragma once

#include <stdint.h>

#define SRT_MS_IN_H 3600000
#define SRT_MS_IN_M 60000
#define SRT_MS_IN_S 1000

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
	uintptr_t		sections_len;
	SrtSection*		sections;
	uintptr_t		str_pool_len;
	uint8_t*		str_pool;
} SrtHandle;

SrtHandle process_srt(const char* path); // from Rust
void free_srt(SrtHandle); // from Rust
bool isTimeInPeriod(const SrtTimeStamp current, const SrtTimePeriod period); // from C
uint32_t getSectionIndexByTime(const SrtHandle* srtHandle, const SrtTimeStamp current); // from C
