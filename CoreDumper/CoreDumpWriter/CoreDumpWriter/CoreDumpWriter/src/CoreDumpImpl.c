#include "CoreDumpImpl.h"
#include "CoreDumpTop.h"
#include "SeparatedFileImplementation.h"
#include "DeflateImplementation.h"
#include "DeltaImplementation.h"

#ifndef BASIC_INTERFACE
#define cd_CreateFile Create
#define  cd_addFrame_P AddCycle
#define  cd_CloseFile Close
#define CoreDumpFile dump_writer
#endif


CoreDumpFile* cd_CreateFile(char * filename){
	//creation du coreTop en utilisant sont créateur par default
	CoreDumpTop* top = cdTop_BlankImplementation(MAX_BLOCK_COUNT_PER_LEVEL);
//création du fichier en le basant sur une implémentation
	CoreDumpFile* test = cdTop_CreateNewDumpFile(top,filename);
	/* */
	cdSepFile_SetTop(test, COMPRESSION_AND_SEPARATION_DEPTH_LEVEL);
#ifdef ACTIVATE_COMPRESSION
	cdDef_Enc_SetTop_func(top, COMPRESSION_AND_SEPARATION_DEPTH_LEVEL);
#endif // ACTIVATE_COMPRESSION
	cdDelta_SetImpl(top);
	return test;
}

void cd_addFrame_F(CoreDumpFile * cdfptr, FILE * frame) {
	cdTop_addFrame_F(cdfptr, frame);
}

void cd_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame) {
	cdTop_addFrame_P(cdfptr, frame, size_frame);
}

void cd_CloseFile(CoreDumpFile * cdfptr) {
	CoreDumpTop* top = ((struct CoreDumpFile_S*)cdfptr)->top;
	cdTop_FinishTree(cdfptr);
	cdTop_CloseDumpFile(cdfptr);
#ifdef ACTIVATE_COMPRESSION
	cdDef_CleanTop(top);
#endif // ACTIVATE_COMPRESSION
	cdDelta_CleanTop(top);
	cdSepFile_CleanTop(top);		
}
