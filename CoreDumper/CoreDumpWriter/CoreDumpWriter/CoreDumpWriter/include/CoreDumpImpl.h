#include<stdint.h>
//configuration de l'implémentation
#define COMPRESSION_AND_SEPARATION_DEPTH_LEVEL 2
#define ACTIVATE_COMPRESSION
#define MAX_BLOCK_COUNT_PER_LEVEL 100
//#define BASIC_INTERFACE

//definition des fonction


#ifdef BASIC_INTERFACE
#pragma once
#include "CoreDumpType.h"
#include "CoreDumpUtils.h"
#include "CoreDumpTop.h"
CoreDumpFile * cd_CreateFile(char * filename);

void cd_addFrame_F(CoreDumpFile * cdfptr, FILE * frame);

void cd_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

void cd_CloseFile(CoreDumpFile * cdfptr);
#else

typedef void dump_writer;
dump_writer* Create(char * filename);

void AddCycle(dump_writer * dump, char * address, int64_t count);

void Close(dump_writer * cdfptr);


#endif //BASIC_INTERFACE
