
#include <time.h>
#include <stdio.h>
#include "CoreDumpConfig.h"
#define printf_if_verbose(format,...) if(VERBOSE)printf(format, __VA_ARGS__);

char * get_time_diff(char * buff, size_t size, struct timespec before,struct timespec after);
