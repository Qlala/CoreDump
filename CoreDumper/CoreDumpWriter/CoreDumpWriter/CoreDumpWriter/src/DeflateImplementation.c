#include "CoreDumpTop.h"
#include "zlib-1.2.11/zlib.h"
#include <string.h>
#include "DeflateImplementation.h"
#include "CoreDumpConfig.h"



#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif






int defImpl_Encode_FF(FILE *source, FILE *dest,int64_t insize,int64_t *outSize)
{
	
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char* in=malloc(CHUNK);
	unsigned char* out= malloc(CHUNK);
	int64_t outsize_loc=0;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, COMPRESSION_LEVEL);
	if (ret != Z_OK)
		goto finally;

	/* compress until end of file */
	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)deflateEnd(&strm);
			ret=Z_ERRNO;
			goto finally;
		}
		flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
		   compression if all of source has been read in */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				printf("error : %s\n", strerror(dest));
				(void)deflateEnd(&strm);
				ret=Z_ERRNO;
				goto finally;
			}
			outsize_loc += have;
		} while (strm.avail_out == 0);
		//assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	//assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	*outSize = outsize_loc;
	ret=Z_OK;
	goto finally;

	finally:
		free(in);
		free(out);
		return ret;
	
}

int defImpl_Encode_PF(char *source, FILE *dest, int64_t inSize, int64_t *outSize)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	int64_t outsize_loc = 0;
	int64_t have_in;
	char* source_st = source;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, COMPRESSION_LEVEL);
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	do {
		have_in = (inSize - ((int64_t)source - (int64_t)source_st)) >= CHUNK ? CHUNK : inSize - ((int64_t)source - (int64_t)source_st);
		strm.avail_in = have_in;
		memcpy(in, source,(size_t) have_in);
		source += CHUNK;
		/*if (ferror(source)) {
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}*/
		flush = ((int64_t)source - (int64_t)source_st) >= inSize ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
		   compression if all of source has been read in */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
			outsize_loc += have;
		} while (strm.avail_out == 0);
		//assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	//assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	*outSize = outsize_loc;
	return Z_OK;

}



int defImpl_Encode_FF_beta(FILE *source, FILE *dest)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, COMPRESSION_LEVEL);
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	do {
		strm.avail_in = (uint32_t)fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}
		flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
		   compression if all of source has been read in */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		//assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	//assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

int defImpl_EncodingNeeded(int depth,void* param) {
	return depth == *(int*) param;
}
void cdDef_CleanTop(CoreDumpTop* cdtptr) {
	free(cdtptr->EncodingNeeeded_param);
}

void cdDef_Enc_SetTop_func(CoreDumpTop* cdtptr,int encode_depth_level) {
	cdtptr->Encode_FF = defImpl_Encode_FF;
	cdtptr->Encode_PF = defImpl_Encode_PF;

	cdtptr->EncodingNeeded = defImpl_EncodingNeeded;
	cdtptr->EncodingNeeeded_param = malloc(sizeof(int));
	*(int*)(cdtptr->EncodingNeeeded_param) = encode_depth_level;
}

