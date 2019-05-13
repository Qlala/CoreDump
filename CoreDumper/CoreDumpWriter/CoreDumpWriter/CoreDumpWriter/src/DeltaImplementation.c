#include "CoreDumpTop.h"
#include "DeltaImplementation.h"
#include <stdio.h>
#include "CoreDumpUtils.h"
//vc diff implementation.


int ImplMyDelta_code_P(
	char*  InFile, int64_t InSize,
	char*  SrcFile,
	FILE* OutFile, int64_t* outSize,
	int BufSize) {
	for (int64_t i = 0;i<InSize; ) {
		//comp dans buffsize;
		int64_t j;
		for(j=0;j<BufSize-sizeof(TYPE_FOR_DELTA_SCAN) && j+i<InSize- sizeof(TYPE_FOR_DELTA_SCAN);j+= sizeof(TYPE_FOR_DELTA_SCAN))//scan à la recherche de différence (méthode efficace)
			if(*(TYPE_FOR_DELTA_SCAN*)(InFile+i+j)!=*(TYPE_FOR_DELTA_SCAN*)(SrcFile + i + j)) goto diff_found;
		for (; j < BufSize && j+i<InSize; j ++)//scan pour les élément pas multiple de 8
			if (*(InFile + i + j) != *(SrcFile + i + j)) goto diff_found;
		*outSize += 1;
		fputc('C',OutFile);//COPY
		i += j;//on passe le bloc
		goto fin_boucle;
	diff_found: 
		*outSize += 1;//on écris le caractère
		fputc('P', OutFile);//provided
		int64_t tocopy = (i + BufSize) > InSize ? InSize - i : BufSize;//attention à la fin de bloc
		*outSize += tocopy;
		fwrite(InFile+i,tocopy  , 1, OutFile);//copy du bloc qui n'est pas conservé
		i+=tocopy;
	fin_boucle:
		;
	}

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
	printf_if_verbose("delta per frame op P updated \n");
	if (refs->reference == NULL || ((int64_t)refs->frame_count - (int64_t)refs->ref_frame_n)!=(int32_t)(refs->frame_count - refs->ref_frame_n)){
		printf_if_verbose("making reference\n");
		cdHeader_setImportant(cdhptr);//setting important
		fputc(0, fst);//pas delta
		cdDelta_MakeReference_P(cdtptr, frame, insize, refs->frame_count);
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
		*size += fwrite(&(ref_f),1, sizeof(int32_t),fst);
		struct timespec t1, t2;
		timespec_get(&t1, TIME_UTC);
		//ImplDelta_code_P(1, frame, insize, refs->reference, refs->ref_size, fst, size,1024);
		ImplMyDelta_code_P( frame, insize, refs->reference, fst, size, DELTA_WINDOW);
		timespec_get(&t2, TIME_UTC);
		printf_if_verbose("delta calcule, de taile %lli , %s\n",*size,get_time_diff(buff_delt,t1,t2));
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
	cdtptr->per_frame_operation_F = cdDelta_per_frame_operation_F;
	cdtptr->per_frame_operation_P = cdDelta_per_frame_operation_P;
	cdtptr->par_frame_operationParam = malloc(sizeof(struct DeltaRefSave));
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	refs->reference = NULL;
}

void cdDelta_CleanTop(CoreDumpTop * cdtptr) {
	cdtptr->per_frame_operation_F ;//remettre a zero
	cdtptr->per_frame_operation_P;
	struct DeltaRefSave* refs = (cdtptr->par_frame_operationParam);
	refs->ref_frame_n = 0;
	refs->frame_count = 0;
	if (refs->reference != NULL)free(refs->reference);
	cdtptr->par_frame_operationParam = NULL;
}