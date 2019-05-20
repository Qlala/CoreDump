#pragma once
#include "zlib-1.2.11/zlib.h"
//Configuration static du program => si library 
//CoreDumpUtils
#define VERBOSE 0
//CoreDumpHeader
#define MARK_CHAR_REF 0xAA
//CoreDumpBlock
//#define USE_THREADED_ENCODE
#define BLOCK_BUFF_SIZE 1024
//DeflateImplementation
#define CHUNK (1<<20)
#define COMPRESSION_LEVEL Z_BEST_SPEED //Z_DEFAULT_COMPRESSION
//deltaImplementation
#define DELTA_THRESHOLD 0.5	//seuil en fraction de la taille 0.01
#define DELTA_WINDOW 64 //1024
#define TYPE_FOR_DELTA_SCAN int64_t
