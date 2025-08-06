#include "../rust/srt_parse/srt_parse.h"

static uint64_t total_ms(const SrtTimeStamp srtTimeStamp) 
{
	return ((uint64_t) srtTimeStamp.h * SRT_MS_IN_H) + ((uint64_t) srtTimeStamp.m * SRT_MS_IN_M) + 
	       ((uint64_t) srtTimeStamp.s * SRT_MS_IN_S) + (uint64_t) srtTimeStamp.ms;
}

bool isTimeInPeriod(const SrtTimeStamp current, const SrtTimePeriod period)
{
	uint64_t start_total_ms = total_ms(period.start);
	uint64_t end_total_ms = total_ms(period.end);
	uint64_t current_total_ms = total_ms(current);

	return (current_total_ms >= start_total_ms) && (current_total_ms <= end_total_ms);
}

uint32_t getSectionIndexByTime(const SrtHandle* srtHandle, const SrtTimeStamp current)
{
	// TODO
	return 0;
}
