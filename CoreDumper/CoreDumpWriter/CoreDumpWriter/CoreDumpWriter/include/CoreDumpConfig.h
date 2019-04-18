#pragma once
//Configuration static du program => si library 

//CoreDumpHeader
#define MARK_CHAR_REF 0xAA
//CoreDumpBlock
#define USE_THREADED_ENCODE
#define BLOCK_BUFF_SIZE 1024
//DeflateImplementation
#define CHUNK (1<<10)
#define COMPRESSION_LEVEL Z_BEST_SPEED 

