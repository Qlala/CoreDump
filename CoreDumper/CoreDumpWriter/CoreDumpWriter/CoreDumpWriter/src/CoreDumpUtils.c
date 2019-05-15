#include "CoreDumpUtils.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

char * get_time_diff(char* buff, struct timespec before, struct timespec after)
{
	uint64_t nsec_diff = after.tv_nsec - before.tv_nsec;
	uint64_t sec_diff = after.tv_sec - before.tv_sec;
	nsec_diff = nsec_diff < 0 ? nsec_diff + 1000000000 : nsec_diff;
	sec_diff = nsec_diff < 0 ? sec_diff - 1 : sec_diff;
	uint64_t mili=(nsec_diff/1000000)%1000;
	uint64_t micro=(nsec_diff/1000)%1000;
	uint64_t nano=nsec_diff%1000;
	uint64_t sec=sec_diff%60;
	uint64_t min=sec_diff/60;
	sprintf(buff, "%llu:%llu.%llu:%llu:%llu", min, sec, mili, micro, nano);
	return buff;
}
