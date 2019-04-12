#pragma once
#include "CoreDumpHeader.h"
#include "CoreDumpType.h"
#include <stdlib.h>
#include <stdio.h>


CoreDumpBlock * cdBlock_CreateNewChild_F(FILE * fst, int64_t  first_frame, int depth_a, int no_write);

void cdBlock_DeleteBlock(CoreDumpBlock * cdbptr);

int cdBlock_addFrame_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, FILE * frame);

int cdBlock_addFrame_P(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, char * frame, int64_t frame_size);

void cdBlock_FinishTree_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst);

void cdBlock_addChildBlock_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE * fst, int depth, int * blockCount, int force_copy);

void cdBlock_WriteFileName_F(CoreDumpBlock * cdbptr, FILE * fst);

void cdBlock_addChildBlockFile_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE ** node_fst, char * nodeFileName, int depth, int * blockCount, int force_copy);
