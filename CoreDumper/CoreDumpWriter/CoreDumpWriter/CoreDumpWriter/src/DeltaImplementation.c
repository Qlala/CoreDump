#include "CoreDumpTop.h"
#include "xdelta-release3_1_apl/xdelta3/xdelta3.h"
#include "DeltaImplementation.h"
#include <stdio.h>
//vc diff implementation.

int ImplDelta_code_F(
	int encode,
	FILE*  InFile,
	FILE*  SrcFile,
	FILE* OutFile,
	int BufSize)
{
	int r, ret;
	//struct stat statbuf;
	xd3_stream stream;
	xd3_config config;
	xd3_source source;
	void* Input_Buf;
	int Input_Buf_Read;

	if (BufSize < XD3_ALLOCSIZE)
		BufSize = XD3_ALLOCSIZE;

	memset(&stream, 0, sizeof(stream));
	memset(&source, 0, sizeof(source));

	xd3_init_config(&config, XD3_NOCOMPRESS|XD3_ADLER32_NOVER);
	config.smatch_cfg = XD3_SMATCH_FASTEST;
	config.winsize = BufSize;
	xd3_config_stream(&stream, &config);

	if (SrcFile)
	{
		//r = fstat(fileno(SrcFile), &statbuf);
		/*if (r)
			return r;*/

		source.blksize = BufSize;
		source.curblk = malloc(source.blksize);

		/* Load 1st block of stream. */
		r = fseek(SrcFile, 0, SEEK_SET);
		if (r)
			return r;
		source.onblk = fread((void*)source.curblk, 1, source.blksize, SrcFile);
		source.curblkno = 0;
		/* Set the stream. */
		xd3_set_source(&stream, &source);
	}

	Input_Buf = malloc(BufSize);

	fseek(InFile, 0, SEEK_SET);
	do
	{
		Input_Buf_Read = fread(Input_Buf, 1, BufSize, InFile);
		if (Input_Buf_Read < BufSize)
		{
			xd3_set_flags(&stream, XD3_FLUSH | stream.flags);
		}
		xd3_avail_input(&stream, Input_Buf, Input_Buf_Read);

	process:
		if (encode)
			ret = xd3_encode_input(&stream);
		else
			ret = xd3_decode_input(&stream);

		switch (ret)
		{
		case XD3_INPUT:
		{
			fprintf(stderr, "XD3_INPUT\n");
			continue;
		}

		case XD3_OUTPUT:
		{
			fprintf(stderr, "XD3_OUTPUT\n");
			r = fwrite(stream.next_out, 1, stream.avail_out, OutFile);
			if (r != (int)stream.avail_out)
				return r;
			xd3_consume_output(&stream);
			goto process;
		}

		case XD3_GETSRCBLK:
		{
			fprintf(stderr, "XD3_GETSRCBLK %qd\n", source.getblkno);
			if (SrcFile)
			{
				r = fseek(SrcFile, source.blksize * source.getblkno, SEEK_SET);
				if (r)
					return r;
				source.onblk = fread((void*)source.curblk, 1,
					source.blksize, SrcFile);
				source.curblkno = source.getblkno;
			}
			goto process;
		}

		case XD3_GOTHEADER:
		{
			fprintf(stderr, "XD3_GOTHEADER\n");
			goto process;
		}

		case XD3_WINSTART:
		{
			fprintf(stderr, "XD3_WINSTART\n");
			goto process;
		}

		case XD3_WINFINISH:
		{
			fprintf(stderr, "XD3_WINFINISH\n");
			goto process;
		}

		default:
		{
			fprintf(stderr, "!!! INVALID %s %d !!!\n",
				stream.msg, ret);
			return ret;
		}

		}

	} while (Input_Buf_Read == BufSize);

	free(Input_Buf);

	free((void*)source.curblk);
	xd3_close_stream(&stream);
	xd3_free_stream(&stream);

	return 0;

};

/*
int main(int argc, char* argv[])
{
	FILE*  InFile;
	FILE*  SrcFile;
	FILE* OutFile;
	int r;

	if (argc != 3) {
		fprintf(stderr, "usage: %s source input\n", argv[0]);
		return 1;
	}

	char *input = argv[2];
	char *source = argv[1];
	const char *output = "encoded.testdata";
	const char *decoded = "decoded.testdata";

	// Encode 

	InFile = fopen(input, "rb");
	SrcFile = fopen(source, "rb");
	OutFile = fopen(output, "wb");

	r = code(1, InFile, SrcFile, OutFile, 0x1000);

	fclose(OutFile);
	fclose(SrcFile);
	fclose(InFile);

	if (r) {
		fprintf(stderr, "Encode error: %d\n", r);
		return r;
	}

	// Decode 

	InFile = fopen(output, "rb");
	SrcFile = fopen(source, "rb");
	OutFile = fopen(decoded, "wb");

	r = code(0, InFile, SrcFile, OutFile, 0x1000);

	fclose(OutFile);
	fclose(SrcFile);
	fclose(InFile);

	if (r) {
		fprintf(stderr, "Decode error: %d\n", r);
		return r;
	}

	return 0;
}
*/
struct DeltaRefSave {
	int64_t frame_count;
	char* reference;
	int64_t ref_size;
	int64_t ref_frame_n;
};
void cdDelta_CountFrame(CoreDumpTop* cdtptr) {
	struct DeltaRefSave* refs = cdtptr->par_frame_operationParam;
	refs->frame_count++;
}


void cdDelta_MakeReference_P(CoreDumpTop* cdtptr, char* frame, int64_t frame_size,int64_t frame_number) {
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	if (refs->reference != NULL)free(refs->reference);
	refs->ref_size = frame_size;
	refs->reference = malloc(refs->ref_size);
	refs->ref_frame_n = frame_number;
	memcpy(refs->reference, frame, refs->ref_frame_n);
}

//TODO
int cdDelta_per_frame_operation_F(FILE*fst,CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, FILE* frame, int64_t* size) {
	printf("delta per frame op F \n");
	fputc(0,fst);//pas un delta
	(*size)++;
	cdDelta_CountFrame(cdtptr);
	//Size doit être changé  pour corresspondre à la taille écrite.
	return 0;
}
int cdDelta_per_frame_operation_P(FILE* fst,CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t* size) {
	cdDelta_CountFrame(cdtptr);
	fputc(0, fst);//pas delta
	(*size)++;
	printf("delta per frame op P \n");
	//Size doit être changé  pour corresspondre à la taille écrite.
	return 0;
}

void cdDelta_SetImpl(CoreDumpTop* cdtptr) {
	cdtptr->per_frame_operation_F = cdDelta_per_frame_operation_F;
	cdtptr->per_frame_operation_P = cdDelta_per_frame_operation_P;
	cdtptr->par_frame_operationParam = malloc(sizeof(struct DeltaRefSave));
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	refs->reference = NULL;
}

void cdDelta_CleanTop(CoreDumpTop * cdtptr) {
	cdtptr->per_frame_operation_F ;
	cdtptr->per_frame_operation_P ;
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	if (refs->reference != NULL)free(refs->reference);
	cdtptr->par_frame_operationParam = NULL;
}