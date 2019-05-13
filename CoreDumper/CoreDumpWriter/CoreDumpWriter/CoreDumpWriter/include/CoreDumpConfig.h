#pragma once
//Configuration static du program => si library 
//CoreDumpUtils
#define VERBOSE 0
//CoreDumpHeader
#define MARK_CHAR_REF 0xAA
//CoreDumpBlock
#define USE_THREADED_ENCODE
#define BLOCK_BUFF_SIZE 1024
//DeflateImplementation
#define CHUNK (1<<10)
#define COMPRESSION_LEVEL Z_DEFAULT_COMPRESSION 
//deltaImplementation
#define DELTA_THRESHOLD 0.01	
#define DELTA_WINDOW 1024
#define TYPE_FOR_DELTA_SCAN int64_t
