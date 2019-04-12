#include "CoreDumpTop.h"
#include "CoreDumpType.h"

char * cdSepFile_gen(int depth, int nb, void * x);

void cdSepFile_SetTop(CoreDumpFile * cdfptr, int file_sep_level);

void cdSepFile_CleanTop(CoreDumpTop * cdtptr);