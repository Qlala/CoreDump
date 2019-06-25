#include "CoreDumpTop.h"
#include "DeltaImplementation.h"
#include <stdio.h>
#include "CoreDumpUtils.h"
#include "CoreDumpConfig.h"
//vc diff implementation.


inline void ImplMyDelta_code_P(char*  InFile, int64_t InSize,char*  SrcFile,FILE* OutFile, int64_t* outSize,int BufSize) {
	int64_t i = 0;
	while (i<InSize) {
		//printf("size=%lli\n", InSize);
		//comp dans buffsize;
		int64_t tocopy = (i + BufSize) > InSize ? InSize - i : BufSize;
		/*
		size_t j=0;
		
		for(j=0;j<(size_t)(BufSize-  (BufSize% sizeof(TYPE_FOR_DELTA_SCAN))) && j+i<(size_t)(InSize-(InSize % sizeof(TYPE_FOR_DELTA_SCAN)));j+= sizeof(TYPE_FOR_DELTA_SCAN))//scan à la recherche de différence (méthode efficace)
			if((*((TYPE_FOR_DELTA_SCAN *)(InFile+i+j)))!=(*((TYPE_FOR_DELTA_SCAN*)(SrcFile+ i + j)))) goto diff_found;
		for (; j <(size_t)BufSize && j+i<(size_t)InSize; j ++)//scan pour les élément pas multiple de la taille du type de scan
			if (InFile[ i + j]!= SrcFile[i + j]) goto diff_found;
		*/
		
		if (memcmp(&InFile[i], &SrcFile[i], (size_t)tocopy) == 0) 
		{
			(*outSize) += 1;
			fputc('C', OutFile);//COPY
			i += BufSize;//on passe le bloc (j)
			//goto fin_boucle;
		}
		else 
		{//diff_found:
			(*outSize)+= 1;//on écris le caractère
			fputc('P', OutFile);//provided
			//attention à la fin de bloc
			 (*outSize) += fwrite(&InFile[i], 1, (size_t)tocopy, OutFile);//copy du bloc qui n'est pas conservé
			i += tocopy;
		}
		//fin_boucle:
		;
	}

}
//char intern_buff[BUFF_SIZE_FOR_DELTA_IO];
inline void ImplMyDelta_code_P_2(const char*  InFile, int64_t InSize, const char*  SrcFile, FILE* OutFile, int64_t* outSize, int BufSize) {
	int64_t i = 0;
	while (i < InSize) {
		int64_t tocopy = (i + BufSize) > InSize ? InSize - i : BufSize;

		if (memcmp(InFile+i, SrcFile+i, (size_t)tocopy) == 0)
		{
			(*outSize) += 1;
			fputc('C', OutFile);//COPY
			
		}
		else
		{
			(*outSize) += 1;//on écris le caractère
			fputc('P', OutFile);//provided
			//for (int j = 0; j < tocopy; j++)fputc(&InFile[i + j], OutFile);
			fwrite(InFile+i, tocopy, 1, OutFile);//copy du bloc qui n'est pas conservé
			(*outSize) += tocopy;
		}
		i += tocopy;
		//
	}
	fflush(OutFile);
}


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
	refs->reference = malloc(refs->ref_size );
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
	printf_if_verbose("delta per frame op P updated \n");
	if (refs->reference == NULL || ((int64_t)refs->frame_count - (int64_t)refs->ref_frame_n)!=(int32_t)(refs->frame_count - refs->ref_frame_n)){
		printf_if_verbose("making reference\n");
		cdHeader_setImportant(cdhptr);//setting important
		fputc(0, fst);//pas delta
		cdDelta_MakeReference_P(cdtptr, frame, insize, refs->frame_count);
		cdDelta_CountFrame(cdtptr);
		(*size)++;
		fwrite(frame,1, insize, fst);
		fflush(fst);
		return 1;
	}
	else {
		
		(*size) = 1;
		int64_t sp=_ftelli64(fst);
		fputc(1, fst);//delta
		int32_t ref_f = (int32_t)(refs->frame_count - refs->ref_frame_n) ;
		*size += fwrite(&(ref_f),1, sizeof(int32_t),fst);
		struct timespec t1, t2;
		#if VERBOSE		timespec_get(&t1, TIME_UTC);
		//ImplDelta_code_P(1, frame, insize, refs->reference, refs->ref_size, fst, size,1024);
		ImplMyDelta_code_P_2( frame, refs->ref_size, refs->reference, fst, size, DELTA_WINDOW);
		timespec_get(&t2, TIME_UTC);
		printf("delta calcule, de taile %lli , %s\n", *size, get_time_diff(buff_delt, 100, t1, t2));
		#else
		ImplMyDelta_code_P_2(frame, refs->ref_size, refs->reference, fst, size, DELTA_WINDOW);
		#endif
		
		//Size doit être changé  pour corresspondre à la taille écrite.
		cdDelta_CountFrame(cdtptr);
		int64_t lp = _ftelli64(fst);
		if (*size != lp - sp) {
			printf_if_verbose("calcul triché => size=%lli != %lli\n",*size, lp - sp);
			*size = lp - sp;//taille exact (TODO => pas triché)
		}
		if ((*size) > insize*DELTA_THRESHOLD) {//seuil passé
			//discarding reference
			if (refs->reference != NULL)free(refs->reference);
			refs->reference = NULL;
			printf_if_verbose("seuil delta atteint avec %lli avec la frame %lli\n", *size, refs->frame_count);
			refs->ref_frame_n = 0;
		}
		else {
			printf_if_verbose("toujours sous le seuil avec size=%lli seuil atteint a %3.2f %%  pour la frame %lli\n", *size,(*size/(insize*DELTA_THRESHOLD))*100.f, refs->frame_count);
			printf_if_verbose("*****ref actuel : %lli******\n", refs->ref_frame_n);
		}
		return 1;
	}
}

void cdDelta_SetImpl(CoreDumpTop* cdtptr) {
	cdtptr->per_frame_operation_P = cdDelta_per_frame_operation_P;
	cdtptr->par_frame_operationParam = malloc(sizeof(struct DeltaRefSave));
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	refs->reference = NULL;
}

void cdDelta_CleanTop(CoreDumpTop * cdtptr) {
	cdtptr->per_frame_operation_P;
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	if (refs->reference != NULL)free(refs->reference);
	cdtptr->par_frame_operationParam = NULL;
}