#pragma once
#include<stdio.h>
#include <stdio.h>
#include "CoreDumpType.h"
#include "CoreDumpBlock.h"


typedef int(*EncodingNeeded_fun_T)(int depth);
typedef int(*MaxBlockCountReach_fun_T)(int depth, int count);
typedef int(*SeparatedFileNeeded_fun_T)(int depth);
typedef char* (*SeperateFileName_fun_T)(int depth, int nb);
typedef int(*Encode_FF_fun_T)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
typedef int(*Decode_FF_fun_T)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
typedef int(*Encode_PF_fun_T)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);
typedef int(*Decode_PF_fun_T)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);
typedef int(*Encode_FP_fun_T)(FILE* input, char** output, int64_t  in_l, int64_t * out_l);
typedef int(*Decode_FP_fun_T)(FILE* input, char** output, int64_t  in_l, int64_t * out_l);
typedef int(*Encode_PP_fun_T)(char* input, char** output, int64_t  in_l, int64_t * out_l);
typedef int(*Decode_PP_fun_T)(char* input, char** output, int64_t  in_l, int64_t * out_l);

int cdTop_TryWaitSema(CoreDumpTop * cdtptr);

CoreDumpFile * cdTop_CreateNewDumpFile(CoreDumpTop * cdtptr, char * filename);

void cdTop_IncSema(CoreDumpTop* cdtptr);

void cdTop_ReleaseSema(CoreDumpTop* cdtptr);

void cdTop_CloseDumpFile(CoreDumpFile * cdfptr);

int cdTop_EncodingNeeded(CoreDumpTop * cdtptr, int depth);

int cdTop_MaxBlockCountReach(CoreDumpTop * cdtptr, int depth, int count);

int cdTop_SeparateFileNeeded(CoreDumpTop * cdtptr, int depth);

char* cdTop_SeparateFileName(CoreDumpTop * cdtptr, int depth, int nb);

void cdTop_FinishTree(CoreDumpFile * cdfptr);

void cdTop_addFrame_F(CoreDumpFile * cdfptr, FILE * frame);

void cdTop_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

//int cdTop_perFrameOp_f(CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, FILE* frame, int64_t size);

//int cdTop_perFrameOp_P(CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t size);

void cdTop_CleanBlankTop(CoreDumpTop * cdtptr);

CoreDumpTop * cdTop_BlankImplementation(int max_block_count);
