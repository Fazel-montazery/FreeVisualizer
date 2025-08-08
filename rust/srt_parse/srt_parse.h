#pragma once

#include <stdint.h>

#define SRT_MS_IN_H 3600000
#define SRT_MS_IN_M 60000
#define SRT_MS_IN_S 1000

#define SRT_S_IN_H 3600
#define SRT_S_IN_M 60

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
	const uint32_t		num;
	const SrtTimePeriod	period;
	const uintptr_t		str_start; // In the str_pool
	const uintptr_t		str_len;
} SrtSection;

typedef struct SrtHandle {
	uintptr_t		sections_len; // Minimum == 1
	SrtSection*		sections;
	uintptr_t		str_pool_len;
	uint8_t*		str_pool;
} SrtHandle;

SrtHandle process_srt(const char* path); // from Rust
void free_srt(SrtHandle); // from Rust
SrtSection* getSectionByTime(const SrtHandle* srtHandle, const SrtTimeStamp current); // from C
SrtTimeStamp convertSecsToSrtTs(int secs); // from C
