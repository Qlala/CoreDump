#include "CoreDumpType.h"
//configuration de l'implémentation
#define COMPRESSION_AND_SEPARATION_DEPTH_LEVEL 2
#define ACTIVATE_COMPRESION
#define MAX_BLOCK_COUNT_PER_LEVEL 10

//definition des fonction

CoreDumpFile * cd_CreateFile(char * filename);

void cd_addFrame_F(CoreDumpFile * cdfptr, FILE * frame);

void cd_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

void cd_CloseFile(CoreDumpFile * cdfptr);
