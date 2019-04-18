#include "CoreDumpTop.h"
#include "xdelta-release3_1_apl/xdelta3/xdelta3.h"
#include "DeltaImplementation.h"
#include <stdio.h>
#include "CoreDumpUtils.h"
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

	xd3_init_config(&config, XD3_JUST_HDR|XD3_SKIP_WINDOW|XD3_SKIP_EMIT |XD3_BEGREEDY |XD3_NOCOMPRESS| XD3_SEC_NOALL|XD3_ADLER32_NOVER| XD3_SMATCH_FASTEST);// XD3_NOCOMPRESS|XD3_ADLER32_NOVER
	//config.smatch_cfg = XD3_SMATCH_FASTEST;
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
			//fprintf(stderr, "XD3_INPUT\n");
			continue;
		}

		case XD3_OUTPUT:
		{
			//fprintf(stderr, "XD3_OUTPUT\n");
			r = fwrite(stream.next_out, 1, stream.avail_out, OutFile);
			if (r != (int)stream.avail_out)
				return r;
			xd3_consume_output(&stream);
			goto process;
		}

		case XD3_GETSRCBLK:
		{
			//fprintf(stderr, "XD3_GETSRCBLK %qd\n", source.getblkno);
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
			//fprintf(stderr, "XD3_GOTHEADER\n");
			goto process;
		}

		case XD3_WINSTART:
		{
		//	fprintf(stderr, "XD3_WINSTART\n");
			goto process;
		}

		case XD3_WINFINISH:
		{
			//fprintf(stderr, "XD3_WINFINISH\n");
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
int ImplMyDelta_code_P(
	char*  InFile, int64_t InSize,
	char*  SrcFile,
	FILE* OutFile, int64_t* outSize,
	int BufSize) {
	for (int64_t i = 0;i<InSize; ) {
		//comp dans buffsize;
		int64_t j;
		for(j=0;j<BufSize-8 && j+i<InSize-8;j+=8)
			if(*(int64_t*)(InFile+i+j)!=*(int64_t*)(SrcFile + i + j)) goto diff_found;
		for (; j < BufSize && j+i<InSize; j ++)
			if (*(InFile + i + j) != *(SrcFile + i + j)) goto diff_found;
		*outSize += 1;
		fputc('C',OutFile);
		i += j;
		goto fin_boucle;
	diff_found: 
		*outSize += 1;
		fputc('P', OutFile);//provided
		int64_t tocopy = (i + BufSize) > InSize ? InSize - i : BufSize;
		*outSize += fwrite(InFile+i,tocopy  , 1, OutFile);
		i+=tocopy;
	fin_boucle:
		;
	}

}



int ImplDelta_code_P(
	int encode,
	char*  InFile,int64_t inSize,
	char*  SrcFile,int64_t srcSize,
	FILE* OutFile,int64_t* outSize,
	int BufSize)
{
	int r, ret;
	//struct stat statbuf;
	xd3_stream stream;
	xd3_config config;
	xd3_source source;
	char* srcFile_pos = SrcFile;
	char* inFile_pos = InFile;

	void* Input_Buf;
	int Input_Buf_Read;

	if (BufSize < XD3_ALLOCSIZE)
		BufSize = XD3_ALLOCSIZE;

	memset(&stream, 0, sizeof(stream));
	memset(&source, 0, sizeof(source));

	xd3_init_config(&config, XD3_NOCOMPRESS);// XD3_NOCOMPRESS|XD3_ADLER32_NOVER
	//config.smatch_cfg = XD3_SMATCH_FASTEST;
	//config.smatch_cfg = XD3_SMATCH_FASTEST;
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
		//r = fseek(SrcFile, 0, SEEK_SET);
		//memcpy(SrcFile)
		/*if (r)
			return r;*/
		//source.onblk = fread((void*)source.curblk, 1, source.blksize, SrcFile);
		memcpy(source.curblk, srcFile_pos, source.blksize);
		source.onblk = source.blksize;
		srcFile_pos += source.blksize;
		source.curblkno = 0;
		/* Set the stream. */
		xd3_set_source(&stream, &source);
	}

	Input_Buf = malloc(BufSize);

	//fseek(InFile, 0, SEEK_SET);
	do
	{
		//Input_Buf_Read = fread(Input_Buf, 1, BufSize, InFile);
		Input_Buf_Read = (BufSize + (inFile_pos - InFile)) > inSize ? inSize-(inFile_pos - InFile) : BufSize;
		memcpy(Input_Buf,inFile_pos,Input_Buf_Read);
		inFile_pos += Input_Buf_Read;
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
			//fprintf(stderr, "XD3_INPUT\n");
			continue;
		}

		case XD3_OUTPUT:
		{
			//fprintf(stderr, "XD3_OUTPUT\n");
			r = fwrite(stream.next_out, 1, stream.avail_out, OutFile);
			*outSize += r;
			if (r != (int)stream.avail_out)
				printf("error delta\n");
			xd3_consume_output(&stream);
			goto process;
		}

		case XD3_GETSRCBLK:
		{
			//fprintf(stderr, "XD3_GETSRCBLK %qd\n", source.getblkno);
			if (SrcFile)
			{
				//r = fseek(SrcFile, source.blksize * source.getblkno, SEEK_SET);
				srcFile_pos =SrcFile+ source.blksize * source.getblkno;
				//if (r)
				//	return r;
				//read
				source.onblk = source.blksize+(source.blksize * source.getblkno) < srcSize ? source.blksize : srcSize - (source.blksize * source.getblkno);
				memcpy(source.curblk, srcFile_pos,source.onblk );
				//source.onblk = fread((void*)source.curblk, 1,
				//	source.blksize, SrcFile);

				source.curblkno = source.getblkno;
			}
			goto process;
		}

		case XD3_GOTHEADER:
		{
			//fprintf(stderr, "XD3_GOTHEADER\n");
			goto process;
		}

		case XD3_WINSTART:
		{
			//fprintf(stderr, "XD3_WINSTART\n");
			goto process;
		}

		case XD3_WINFINISH:
		{
			//fprintf(stderr, "XD3_WINFINISH\n");
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
	memcpy(refs->reference, frame, refs->ref_size);
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
char buff_delt[100];
int cdDelta_per_frame_operation_P(FILE* fst,CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t* size) {
	
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	
	int64_t insize = *size;
	printf("delta per frame op P updated \n");
	if (refs->reference == NULL) {
		printf("making reference\n");
		fputc(0, fst);//pas delta
		cdDelta_MakeReference_P(cdtptr, frame, *size, refs->frame_count);
		cdDelta_CountFrame(cdtptr);
		(*size)++;
		fwrite(frame, insize, 1, fst);
		return 1;
	}
	else {
		
		*size = 1;
		int64_t sp=_ftelli64(fst);
		fputc(1, fst);//delta
		int32_t ref_f = refs->frame_count - refs->ref_frame_n ;
		*size += fwrite(&(ref_f),sizeof(int32_t),1,fst);
		struct timespec t1, t2;
		timespec_get(&t1, TIME_UTC);
		//ImplDelta_code_P(1, frame, insize, refs->reference, refs->ref_size, fst, size,1024);
		ImplMyDelta_code_P( frame, insize, refs->reference, fst, size, 1024);
		timespec_get(&t2, TIME_UTC);
		printf("delta calcule, de taile %lli , %s\n",*size,get_time_diff(buff_delt,t1,t2));
		//Size doit être changé  pour corresspondre à la taille écrite.
		cdDelta_CountFrame(cdtptr);
		int64_t lp = _ftelli64(fst);
		*size = lp - sp;//taille exact (TODO => pas triché)
		return 1;
	}
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