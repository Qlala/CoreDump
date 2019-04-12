#pragma once
#include "CoreDumpHeader.h"
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32



struct CoreDumpBlock_S
{
	coreDumpHeader* header_ptr;
	struct CoreDumpBlock_S* child;
	FILE* nodeFile;
	char* nodeFileName;
	int depth;
	int blockCount;
};
typedef struct CoreDumpBlock_S CoreDumpBlock;
typedef struct CoreDumpTop_S CoreDumpTop;
struct CoreDumpTop_S
{
	void* EncodingNeeeded_param;
	int(*EncodingNeeded)(int depth,void* param);
	void* MaxBlockCountReach_param;
	int(*MaxBlockCountReach)(int depth, int count,void* param);
	void* SepFileParam_SeparateFileNeeded;
	int(*SeparateFileNeeded)(int depth,void* param);
	void* SepFileParam_SeparateFileName;
	char* (*SeparateFileName)(int depth, int nb,void*param);
	//encode decode
	int(*Encode_FF)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
	int(*Decode_FF)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
	int(*Encode_PF)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);
	int(*Decode_PF)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);
	int(*Encode_FP)(FILE* input, char* output, int64_t  in_l, int64_t * out_l);
	int(*Decode_FP)(FILE* input, char* output, int64_t  in_l, int64_t * out_l);
	int(*Encode_PP)(char* input, char* output, int64_t  in_l, int64_t * out_l);
	int(*Decode_PP)(char* input, char* output, int64_t  in_l, int64_t * out_l);
	//
	//return 1 if copy is not needed
	void* par_frame_operationParam;
	int (*per_frame_operation_F)(FILE* dst,CoreDumpHeader* cdhptr,CoreDumpTop* cdtptr,FILE* frame,int64_t* size);
	int(*per_frame_operation_P)(FILE* dst,CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t* size);
	//Semaphore
#ifdef _WIN32
	HANDLE protection_Mutex;
	volatile int64_t semaphore;
#else
	#error version pour Pthread

#endif // _WIN32

};

typedef struct CoreDumpFile_S CoreDumpFile;
struct CoreDumpFile_S
{
	FILE* file;
	char* fileName;
	CoreDumpTop* top;
	CoreDumpBlock* tree;

};
