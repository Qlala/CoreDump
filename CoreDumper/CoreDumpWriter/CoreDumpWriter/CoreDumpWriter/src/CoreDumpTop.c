#include "CoreDumpType.h"
#include "CoreDumpTop.h"
#include "CoreDumpHeader.h"
#include <string.h>
#include <stdio.h>
#include <io.h>
#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32


void cdTop_InitSemaphore(CoreDumpTop* cdtptr) {
#ifdef _WIN32
	cdtptr->protection_Mutex = CreateMutex(0, 0, 0);
	cdtptr->semaphore = 0;
#else
#error "A implémenter avec Pthread"
#endif // _WIN32


}

void cdTop_IncSema(CoreDumpTop* cdtptr)
{
#ifdef _WIN32
	WaitForSingleObject(cdtptr->protection_Mutex, INFINITE);
	cdtptr->semaphore++;
	ReleaseMutex(cdtptr->protection_Mutex);
#else
#error "pas fait"
#endif // _WIN32


}
void cdTop_ReleaseSema(CoreDumpTop* cdtptr) 
{
#ifdef _WIN32
	WaitForSingleObject(cdtptr->protection_Mutex, INFINITE);
	cdtptr->semaphore--;
	ReleaseMutex(cdtptr->protection_Mutex);
#else
#error "pas fait"
#endif // _WIN32
}
void cdTop_WaitSema(CoreDumpTop* cdtptr) 
{
#ifdef _WIN32

	do{
		ReleaseMutex(cdtptr->protection_Mutex);	
		Sleep(1);
		WaitForSingleObject(cdtptr->protection_Mutex, INFINITE);
	} while (cdtptr->semaphore != 0);

#else
#error "Pthread"
#endif // _WIN32


}

CoreDumpFile* cdTop_CreateNewDumpFile(CoreDumpTop* cdtptr, char* filename) 
{
	CoreDumpFile* cdf = (CoreDumpFile*)malloc(sizeof(CoreDumpFile));
	
	cdf->fileName = malloc(strlen(filename)+1);
	strcpy_s(cdf->fileName, strlen(filename)+1, filename);
	fopen_s(&cdf->file,cdf->fileName, "wb+");
	cdf->top = cdtptr;
	cdf->tree = cdBlock_CreateNewChild_F(cdf->file, 0, 0, 0);
	return cdf;
}
int cdTop_MoveFile(char* src,char*dst) {
#ifdef _WIN32
	DeleteFileA(dst);
	MoveFileA(src, dst);
#else
	remove(dst);
	rename(src, dst);
#endif // _WIN32


}



void cdTop_FreeTop(CoreDumpTop* cdtptr) {
	free(cdtptr);
}


void cdTop_CloseDumpFile(CoreDumpFile* cdfptr) {
	cdTop_WaitSema(cdfptr->top);
	fclose(cdfptr->file);
	free(cdfptr->fileName);
	cdBlock_DeleteBlock(cdfptr->tree);
	//free(cdfptr->top);
}
int cdTop_EncodingNeeded(CoreDumpTop* cdtptr, int depth) {
	return cdtptr->EncodingNeeded(depth, cdtptr->EncodingNeeeded_param);
}
int cdTop_MaxBlockCountReach(CoreDumpTop* cdtptr, int depth,int count) {
	return cdtptr->MaxBlockCountReach(depth,count, cdtptr->MaxBlockCountReach_param);
}
int cdTop_SeparateFileNeeded(CoreDumpTop* cdtptr, int depth) {
	return cdtptr->SeparateFileNeeded(depth, cdtptr->SepFileParam_SeparateFileNeeded);
}
char* cdTop_SeparateFileName(CoreDumpTop* cdtptr, int depth,int nb) {
	char* temp=cdtptr->SeparateFileName(depth,nb ,cdtptr->SepFileParam_SeparateFileName);
	return temp;
}


void cdTop_rebaseTree(CoreDumpFile* cdfptr) {
	cdHeader_goStartIndex_F(cdfptr->tree->header_ptr,cdfptr->file);
	CoreDumpBlock* temp = cdBlock_CreateNewChild_F(cdfptr->file, 0, cdfptr->tree->depth + 1, 1);
	if (cdTop_SeparateFileNeeded(cdfptr->top,temp->depth)) {//on doit crée un nouveau fichier s&paré
		cdHeader_SetExternFile(temp->header_ptr);
		fclose(cdfptr->file);
		//nodefilename = NULL car il temp vie d'etre créer
		temp->nodeFileName = cdTop_SeparateFileName(cdfptr->top,temp->depth, temp->blockCount);//on génère le nom du fichier
		cdTop_MoveFile(cdfptr->fileName, temp->nodeFileName);//on l'échange avec le fichier qui contenait l'arbre
		fopen_s(&temp->nodeFile ,temp->nodeFileName, "rb");//on rouvre le fichier qui contient l'abre d'avant
		fopen_s(&cdfptr->file,cdfptr->fileName, "wb+");//on créer un nouveau fichier pour avoir l'abre plus haut
		cdHeader_UpdateHeader(temp->header_ptr, cdfptr->file);
		cdBlock_WriteFileName_F(temp, cdfptr->file);//on écris le nom du fichier de l'abre d'avant dans le nouvelle arbre
		cdBlock_addChildBlockFile_F(cdfptr->tree->header_ptr, temp->header_ptr, cdfptr->top, &temp->nodeFile, temp->nodeFileName, temp->depth, &temp->blockCount, 0);//on ajoute effectivement l'ancienne arbre(possiblement encode)
		cdBlock_DeleteBlock(cdfptr->tree);
		cdfptr->tree = temp;//temp est le nouvelle arbre(branche)
		if(cdfptr->tree->nodeFile!=NULL)fclose(cdfptr->tree->nodeFile);
		cdfptr->tree->nodeFile = NULL;

		//cdHeader_UpdateHeader(cdfptr->tree->header_ptr, cdfptr->file);
		cdfptr->tree->child = cdBlock_CreateNewChild_F(cdfptr->file,cdfptr->tree->header_ptr->lastFrame, cdfptr->tree->depth - 1, 0);//temp n'as pas de child car créer en mode no write
	}
	else{//pas de fichier séparé
		cdBlock_addChildBlock_F(cdfptr->tree->header_ptr, temp->header_ptr, cdfptr->top, cdfptr->file, temp->depth, &temp->blockCount, 1);
		cdHeader_UpdateHeader(temp->header_ptr, cdfptr->file);
		//cdHeader_BlockEnd(temp->header_ptr);
		cdBlock_DeleteBlock(cdfptr->tree);
		cdfptr->tree = temp;
		
		cdfptr->tree->child = cdBlock_CreateNewChild_F(cdfptr->file, cdfptr->tree->header_ptr->lastFrame, cdfptr->tree->depth - 1, 0);//temp n'as pas de child car créer en mode no write
	}
	printf("arbre rebase\n");
}

void cdTop_FinishTree(CoreDumpFile* cdfptr) {
	cdBlock_FinishTree_F(cdfptr->tree, cdfptr->top, cdfptr->file);
	cdTop_rebaseTree(cdfptr);
}

void cdTop_addFrame_F(CoreDumpFile* cdfptr,FILE* frame)
{ 
	if (cdBlock_addFrame_F(cdfptr->tree, cdfptr->top, cdfptr->file,frame) )
	{
		cdTop_rebaseTree(cdfptr);
	}
}
void cdTop_addFrame_P(CoreDumpFile* cdfptr, char* frame,int64_t size_frame)
{
	if (cdBlock_addFrame_P(cdfptr->tree, cdfptr->top, cdfptr->file, frame,size_frame))
	{
		cdTop_rebaseTree(cdfptr);
	}
}

/*
not needed
int cdTop_perFrameOp_f(CoreDumpHeader * cdhptr, CoreDumpTop * cdtptr, FILE * frame, int64_t size)
{
	cdtptr->per_frame_operation_F(cdhptr,cdtptr,frame,size,)
	return 0;
}

int cdTop_perFrameOp_P(CoreDumpHeader * cdhptr, CoreDumpTop * cdtptr, char * frame, int64_t size)
{
	return 0;
}
*/
int bImpl_MaxBlockCountReach(int depth, int count,void* param) {
	return count >= *(int*)param;
}
int bImpl_EncodingNeeded(int depth,void* param) {
	return 0;
}
int bImpl_SeparateFileNeeded(int depth,void* param) {
	return 0;
}
int bImpl_per_frame_operation_F(CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, FILE* frame, int64_t* size){
	printf("default per frame op F \n");
	//Size doit être changé  pour corresspondre à la taille écrite.
	return 0;
}
int bImpl_per_frame_operation_P(CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t* size) {
	printf("default per frame op P \n");
	//Size doit être changé  pour corresspondre à la taille écrite.
	return 0;
}


void cdTop_CleanBlankTop(CoreDumpTop* cdtptr) {
	if (cdtptr->MaxBlockCountReach_param != NULL)free(cdtptr->MaxBlockCountReach_param);
}

CoreDumpTop* cdTop_BlankImplementation(int max_block_count) {
	CoreDumpTop* cdt = malloc(sizeof(CoreDumpTop));
	cdt->MaxBlockCountReach_param = malloc(sizeof(int));
	*(int*)cdt->MaxBlockCountReach_param = max_block_count;
	cdt->MaxBlockCountReach = bImpl_MaxBlockCountReach;
	cdt->EncodingNeeded = bImpl_EncodingNeeded;
	cdt->SeparateFileNeeded = bImpl_SeparateFileNeeded;
	cdt->per_frame_operation_F=bImpl_per_frame_operation_F;
	cdt->per_frame_operation_P = bImpl_per_frame_operation_P;
	cdTop_InitSemaphore(cdt);
	return cdt;
}
