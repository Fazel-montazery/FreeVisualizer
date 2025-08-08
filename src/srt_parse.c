#include "../rust/srt_parse/srt_parse.h"

// To optimize the binary search in getSectionByTime()
static SrtTimePeriod preTimePeriod = { 0 };
static uint32_t preSectionIndex = 0;

static uint64_t total_ms(const SrtTimeStamp srtTimeStamp) 
{
	return ((uint64_t) srtTimeStamp.h * SRT_MS_IN_H) + ((uint64_t) srtTimeStamp.m * SRT_MS_IN_M) + 
	       ((uint64_t) srtTimeStamp.s * SRT_MS_IN_S) + (uint64_t) srtTimeStamp.ms;
}

// return 0 if it's in, -1 if it's behind and 1 if it's in front
static int isTimeInPeriod(const SrtTimeStamp current, const SrtTimePeriod period)
{
	uint64_t start_total_ms = total_ms(period.start);
	uint64_t end_total_ms = total_ms(period.end);
	uint64_t current_total_ms = total_ms(current);

	if (current_total_ms < start_total_ms)
		return -1;
	else if (current_total_ms > end_total_ms)
		return 1;
	else
		return 0;
}

// Simple Binary search by time
// No null check cause it's going to be called in a loop (for performance)
// Also no null return, just returning the first element when nothing found
SrtSection* getSectionByTime(const SrtHandle* srtHandle, const SrtTimeStamp current)
{
	// ignore the computation if we are already in the period
	if (isTimeInPeriod(current, preTimePeriod) == 0)
		return &(srtHandle->sections[preSectionIndex]);

	uint32_t start = 0;
	uint32_t end = srtHandle->sections_len - 1;

	while (start <= end) {
		uint32_t mid = start + (end - start) / 2;
		
		int res = isTimeInPeriod(current, srtHandle->sections[mid].period);
		if (res == -1) {
			end = mid - 1;
		} else if (res == 1) {
			start = mid + 1;
		} else {
			preTimePeriod = srtHandle->sections[mid].period;
			preSectionIndex = mid;
			return &(srtHandle->sections[mid]);
		}
	}

	preTimePeriod = srtHandle->sections[0].period;
	preSectionIndex = 0;
	return &(srtHandle->sections[0]);
}

// convert seconds to SrtTimeStamp
SrtTimeStamp convertSecsToSrtTs(int secs)
{
	SrtTimeStamp result;
	result.h = secs / SRT_S_IN_H;
	secs -= result.h * SRT_S_IN_H;
	result.m = secs / SRT_S_IN_M;
	secs -= result.m * SRT_S_IN_M;
	result.s = secs;
	result.ms = 0;
	return result;
}
