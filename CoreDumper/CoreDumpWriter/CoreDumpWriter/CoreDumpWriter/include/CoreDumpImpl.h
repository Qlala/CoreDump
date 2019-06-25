#include<stdint.h>
//configuration de l'implémentation
#define COMPRESSION_AND_SEPARATION_DEPTH_LEVEL 3// nombre de niveau avant de faire un fichier séparé et séparée (niveau 0 compte)
#define ACTIVATE_COMPRESSION //active la compression ( codeur enthropique)
#define MAX_BLOCK_COUNT_PER_LEVEL 100//nombre d'enfant pas parent
//#define BASIC_INTERFACE

//definition des fonction
//TODO format des nomber des fichier.

#ifdef BASIC_INTERFACE
#pragma once
#include "CoreDumpType.h"
#include "CoreDumpUtils.h"
#include "CoreDumpTop.h"
CoreDumpFile * cd_CreateFile(char * filename);


void cd_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

void cd_CloseFile(CoreDumpFile * cdfptr);
#else

typedef void dump_writer;
dump_writer* Create(char * filename);

void AddCycle(dump_writer * dump, char * address, int64_t count);

void Close(dump_writer * cdfptr);


#endif //BASIC_INTERFACE
