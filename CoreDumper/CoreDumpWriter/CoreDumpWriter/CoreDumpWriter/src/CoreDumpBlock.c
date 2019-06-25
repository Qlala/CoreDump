#include "CoreDumpUtils.h"
#include "CoreDumpHeader.h"
#include "CoreDumpType.h"
#include "CoreDumpBlock.h"
#include "CoreDumpTop.h"
#include "CoreDumpConfig.h"
#include <stdlib.h>
#include<string.h>
#ifdef _WIN32
	#include <Windows.h>
#else
	#include <pthread.h>//librairie pthread 
#endif // _WIN32


#include <time.h>
//buffer pour la copie
char block_buff[BLOCK_BUFF_SIZE];
//buffer pour les chaines
char* string_buff[255];

//--------------------------------------------------------------------------
//Fonction de création
//---------------------------------------------------------------------------
//Ces fonction créer des block
//fonction par défault
CoreDumpBlock* cdBlock_Create(FILE* fst, int64_t  first_frame, int no_write) {
	CoreDumpBlock* cdb = (CoreDumpBlock*)malloc(sizeof(CoreDumpBlock));
	cdb->child = NULL;
	cdb->nodeFile = NULL;
	cdb->nodeFileName = NULL;
	cdb->blockCount = 0;
	cdb->header_ptr = cdHeader_Create(_ftelli64(fst));
	cdb->header_ptr->firstFrame = first_frame;
	cdb->depth = 0;
	if (!no_write)cdHeader_UpdateHeader_no_restore(cdb->header_ptr, fst);
	return cdb;
}
//création de feuille
CoreDumpBlock* cdBlock_CreateLeaf_F(FILE* fst, int64_t  first_frame,int no_write) {
	CoreDumpBlock* cdb = (CoreDumpBlock*)malloc(sizeof(CoreDumpBlock));
	cdb->child = NULL;
	cdb->nodeFile = NULL;
	cdb->nodeFileName = NULL;
	cdb->blockCount = 0;
	cdb->header_ptr = cdHeader_Create(_ftelli64(fst));
	cdb->header_ptr->firstFrame = first_frame;
	cdb->header_ptr->configuration |= BASE;//mis en mode base
	cdb->depth = 0;
	if (!no_write)cdHeader_UpdateHeader_no_restore(cdb->header_ptr, fst);
	return cdb;
}
//création d'arbre
CoreDumpBlock* cdBlock_CreateTree_F(FILE* fst,int64_t  first_frame,int depth_a,int no_write)
{
	CoreDumpBlock* cdb = (CoreDumpBlock*)malloc(sizeof(CoreDumpBlock));
	//Chechking malloc succes
	if (cdb == NULL) {
		printf("malloc failure\n");
	}
	cdb->blockCount = 0;
	cdb->header_ptr = cdHeader_Create(_ftelli64(fst));
	cdb->header_ptr->firstFrame = first_frame;
	cdb->depth = depth_a;
	cdb->child = NULL;
	cdb->nodeFile = NULL;
	cdb->nodeFileName = NULL;
	if (!no_write) 
	{
		cdHeader_UpdateHeader_no_restore(cdb->header_ptr, fst);
		cdb->child = cdBlock_CreateNewChild_F(fst, first_frame, depth_a-1,0);
	}
	return cdb;
}
//supprime les information concernant les fichier extern d'une structure CoreDumpBlock
void cdBlock_CleanChildFile_F(CoreDumpBlock* cdbptr) {
	if (cdbptr->nodeFile != NULL) {
		fclose(cdbptr->nodeFile);
	}
	if (cdbptr->nodeFileName != NULL)free(cdbptr->nodeFileName);//pas de fuite
	cdbptr->nodeFileName = NULL;
	cdbptr->nodeFile = NULL;
}

//Créer un nouveau fichier extern pour le block cdbptr qui aurrait du se trouver dans fst
void cdBlock_CreateNewChildFile_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE* fst)
{
	cdBlock_CleanChildFile_F(cdbptr);//néttoie le block
	//gènere le nouveau nom
	cdbptr->nodeFileName =cdTop_SeparateFileName(cdtptr,cdbptr->depth, cdbptr->blockCount);
	remove(cdbptr->nodeFileName);//supression du fichier si il existe déja
	fopen_s(&cdbptr->nodeFile, cdbptr->nodeFileName, "wb+");//ouvre le fichier

	printf_if_verbose("nouveau fichier creer : %s\n", cdbptr->nodeFileName);
	if (!cdbptr->nodeFile) {
		printf("error création de fichier \n");
	}
	//écrit le nom du fichier enfant dans le fichier parent
	cdBlock_WriteFileName_F(cdbptr, fst);
}
//création automatique de block enfant
CoreDumpBlock* cdBlock_CreateNewChild_F(FILE* fst, int64_t  first_frame, int depth_a,int no_write)
{
	printf_if_verbose("Creation  d'enfant : premiere frame %lli depth=%i en %lli avec no_write=%i\n" , first_frame, depth_a,fst!=NULL? _ftelli64(fst):0,no_write);
	if (depth_a > 0) {//si non null alors ce n'est pas une feuille
		return cdBlock_CreateTree_F(fst, first_frame, depth_a,no_write);
	}
	else 
	{
		return cdBlock_CreateLeaf_F(fst, first_frame,no_write);
	}
}
void cdBlock_DeleteBlock(CoreDumpBlock** cdbptr) {
	if ((*cdbptr)->child != NULL)cdBlock_DeleteBlock(&(*cdbptr)->child);
	cdHeader_Delete((*cdbptr)->header_ptr);
	cdBlock_CleanChildFile_F(*cdbptr);
	*cdbptr = NULL;
}
//propage la marqueur important au parent.
//cdbptr est le parent
void cdBlock_propagateImportant(CoreDumpBlock* cdbptr) {
	if (cdbptr->child) {
		if (cdHeader_isImportant(cdbptr->child->header_ptr)) {
			cdHeader_setImportant(cdbptr->header_ptr);
		}
	}
}




//--------------------------------------------
//--threaded Encode
//--------------------------------------------
struct dataThreadEncode
{
	CoreDumpTop* cdtptr;
	int64_t inSize;
	int64_t* outSize;
	char* input_name;
	char * output_name;
#ifdef _WIN32
	HANDLE* hThread;
#else
	pthread_t thread;
#endif // _WIN32	
};
#ifdef _WIN32
//fonction pour le multi threading sous Win32
DWORD WINAPI ThreadProc_Encode(LPVOID lpParameter) {
	struct dataThreadEncode* param = lpParameter;
	HANDLE hThread = *(param->hThread);
	
	cdTop_IncSema(param->cdtptr);
	FILE * input = fopen(param->input_name, "rb");
	FILE * output = fopen(param->output_name, "wb");
	if (!input) {
		fprintf(stderr, "error opening %s: %s \n", param->input_name, strerror(errno));
	}
	if (!output) {
		fprintf(stderr, "error opening %s: %s \n", param->output_name, strerror(errno));
	}
	param->cdtptr->Encode_FF(input, output, param->inSize, param->outSize);
	cdTop_ReleaseSema(param->cdtptr);
	
	do {
		fclose(input);
		fclose(output);
		remove(param->input_name);
	}while (rename(param->output_name, param->input_name) != 0);
	free(param->output_name);
	free(param->input_name);
	free(param->hThread);
	free(param);
	CloseHandle(hThread);
	return 0;
}
#else
//fonction pour le multithreading en pthread (linux)
int ThreadProc_Encode(void* lpParameter)

	struct dataThreadEncode* param = lpParameter;
	pthread_t hThread = (param->hThread);
	//printf("Thread de compression lancé de %s vers %s\n", param->input_name, param->output_name);
	cdTop_IncSema(param->cdtptr);
	FILE * input = fopen(param->input_name, "rb");
	FILE * output = fopen(param->output_name, "wb");
	if (!input) {
		fprintf(stderr, "error opening %s: %s \n", param->input_name, strerror(errno));
	}
	if (!output) {
		fprintf(stderr, "error opening %s: %s \n", param->output_name, strerror(errno));
	}
	param->cdtptr->Encode_FF(input, output, param->inSize, param->outSize);
	cdTop_ReleaseSema(param->cdtptr);
	
	do {
		fclose(input);
		fclose(output);
		remove(param->input_name);
	} while (rename(param->output_name, param->input_name) != 0);
	free(param->output_name);
	free(param->input_name);
	free(param->hThread);
	free(param);
	return 0;
}
	#endif // _WIN32



//lance le thread d'encodage
void cdBlock_ThreadedEncode_FF(CoreDumpTop* cdtptr,char* input_name,char* output_name ,FILE** input, FILE** output, int64_t inSize, int64_t* outSize) 
{
	struct dataThreadEncode* param=malloc(sizeof(struct dataThreadEncode));
	param->cdtptr = cdtptr;
	param->inSize = inSize;
	param->outSize = outSize;
	param->input_name = malloc(strlen(input_name)+1);
	strcpy(param->input_name, input_name);
	param->output_name = output_name;
	param->hThread = malloc(sizeof(HANDLE));
	fclose(*input);
	fclose(*output);
	printf_if_verbose("Thread de compression lancé de %s vers %s\n", param->input_name, param->output_name);
	#ifdef _WIN32
		*(param->hThread)=CreateThread(NULL, 0, ThreadProc_Encode, param, 0,0);
	#else
		pthread_create(param->thread, NULL, ThreadProc_Encode, param);
	#endif
	//on prend le controle des fichier

	*input = NULL;
	*output = NULL;

}


//------------------------------------------------------
//fonction d'ajout de block
//---------------------------------------------------
void cdBlock_addChildBlockFile_F(coreDumpHeader* src_cdptr, coreDumpHeader* dst_cdptr, CoreDumpTop* cdtptr, FILE** node_fst, char* nodeFileName, int depth, int* blockCount, int force_copy)//F correspond au fait que fst soit un fichier
{
	// /!\ fst have to exist => fait attentioon
	if (cdTop_EncodingNeeded(cdtptr, depth) || cdHeader_IsCompressed(dst_cdptr))//on doit encoder ?
	{
		cdHeader_SetCompressed(dst_cdptr);

		fclose(*node_fst);//flush everything
		char* enc_node_name = malloc(strlen(nodeFileName) + 10);
		strcpy(enc_node_name, nodeFileName);
		strcat(enc_node_name, ".encode");
		remove(enc_node_name);
		FILE* temp_node;
		fopen_s(node_fst, nodeFileName, "rb");
		fopen_s(&temp_node, enc_node_name, "wb");//safe open=> node_fst is a ptr to FILE*
		if (!*node_fst) {
			fprintf(stderr, "error opening %s: %s", nodeFileName, strerror(errno));
		}
		if (!temp_node) {
			fprintf(stderr, "error opening %s: %s", enc_node_name, strerror(errno));
		}
		//_fseeki64(0, temp_node,SEEK_SET);
		int64_t  size;


		#ifndef USE_THREADED_ENCODE
			struct timespec before, after;
			timespec_get(&before, TIME_UTC);
			printf_if_verbose("Compression en cours \n");
			cdtptr->Encode_FF(*node_fst, temp_node, src_cdptr->totalSize, &size);
			timespec_get(&after, TIME_UTC);
			printf_if_verbose("Compression de %llu a %llu en %s\n", src_cdptr->totalSize, size,get_time_diff(string_buff,255,before,after));
		#else
			cdBlock_ThreadedEncode_FF(cdtptr, nodeFileName,enc_node_name, node_fst, &temp_node, src_cdptr->totalSize, &size);
		#endif
		int64_t  start_pos = cdHeader_BlockEnd(dst_cdptr);

		cdHeader_addBlockSize(dst_cdptr, start_pos, strlen(nodeFileName) + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
		#ifndef USE_THREADED_ENCODE
			fclose(*node_fst);
			fclose(temp_node);
			remove(nodeFileName);
			rename(enc_node_name, nodeFileName);
			free(enc_node_name);
		#endif	//on ne free rien sinon puisque l'on donne le controle au thread de compression
			printf_if_verbose("arbre (d=%i) contenant les frames %lli à  %lli encodé à la position : %lli réduit de %lli à %lli du fichier %s\n", depth, src_cdptr->firstFrame, src_cdptr->lastFrame, src_cdptr->lastAddedBlockPos, src_cdptr->totalSize, size, nodeFileName);

	}
	else
	{
		//le block est déja dans le fichier
		int64_t  start_pos = dst_cdptr->startPosition + dst_cdptr->totalSize;
		cdHeader_addBlockSize(dst_cdptr, start_pos, strlen(nodeFileName) + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);



	}
	*blockCount++;//on a un block de plus
}

//DST doit être différent de SRC(fichier et header)
void cdBlock_addChildBlockCopy_F(coreDumpHeader* src_cdptr, coreDumpHeader* dst_cdptr, CoreDumpTop* cdtptr, FILE* src_fst, FILE* dst_fst, int depth, int* blockCount) {
	//as-ton besoin de compresser le block
	if (cdTop_EncodingNeeded(cdtptr, depth) || cdHeader_IsCompressed(dst_cdptr))//on doit encoder ?
	{
		cdHeader_SetCompressed(dst_cdptr);//marque que le block est compressé dans son header

		cdHeader_goStartIndex_F(src_cdptr, src_fst);//on se place au début
		int64_t  start_pos = _ftelli64(src_fst);

		int64_t  size;
		_fseeki64(dst_fst, dst_cdptr->startPosition + dst_cdptr->totalSize, SEEK_SET);//on se met la ou on doit ajouter le block encodé
		
		cdtptr->Encode_FF(src_fst, dst_fst, src_cdptr->totalSize, &size);//on encode directement au bonne endroit
		cdHeader_BlockMarker_F(dst_fst);//on met un marker
		cdHeader_addBlockSize(dst_cdptr, start_pos, size + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);//la taille à changé puisqu'on encode 
	}
	else//non
	{
			cdHeader_goStartIndex_F(src_cdptr, src_fst);//on se place au début
			//taille :
			int64_t  size = src_cdptr->totalSize;
			//on détermine les positions pour l'écriture et la lecture:
			int64_t src_pos = _ftelli64(src_fst);
			int64_t  dst_pos = cdHeader_BlockEnd(dst_cdptr);

			_fseeki64(dst_fst, dst_pos, SEEK_SET);
			int i;
			fflush(src_fst);
			//copy src_fst->dst_fst
			for (i = 0; i < (size - BLOCK_BUFF_SIZE); i += BLOCK_BUFF_SIZE) {
				fread(block_buff, 1, BLOCK_BUFF_SIZE, src_fst);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, dst_fst);
			}
			fread(block_buff, 1, (size_t)(size - i), src_fst);
			fwrite(block_buff, 1, (size_t)(size - i), dst_fst);
			
			fflush(dst_fst);
			printf_if_verbose("copie d'un bloc en %lli de taille %lli fin en %lli \n", dst_pos, size, _ftelli64(dst_fst));
			//cdHeader_BlockMarker_F(fst);// pas marker de début de block
			//ajout des donnés du block recopier dans le header
			cdHeader_addBlockSize(dst_cdptr, dst_pos, src_cdptr->totalSize, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
	}
	(*blockCount)++;//on a un block de plus

}

//Ajout des données 
//src_cdptr: header source
//dst_cdptr : header destination
//fst : fichier source et destination
//blockcount : compte actuel des blocks de la destination
//depth : profondeur de la destination
//force_copy : force la copie (débugage => peu utiliser)
void cdBlock_addChildBlock_F(coreDumpHeader* src_cdptr, coreDumpHeader* dst_cdptr, CoreDumpTop* cdtptr, FILE* fst, int depth, int* blockCount, int force_copy)
{
	//doit-on compresser ?
	if (cdTop_EncodingNeeded(cdtptr,depth) || cdHeader_IsCompressed(dst_cdptr))//on doit encoder ?
	{
		//on met le marker de compression sur le header
		cdHeader_SetCompressed(dst_cdptr);

		cdHeader_goStartIndex_F(src_cdptr, fst);//b.GoStartIndex(bl_st);
		int64_t  start_pos = _ftelli64(fst);
		//fichier temporaire pour la compression
		FILE* compress_c;
		tmpfile_s(&compress_c);

		int64_t  size;
		//compression
		cdtptr->Encode_FF(fst, compress_c, src_cdptr->totalSize, &size);
		//on se replace à la bonne position
		_fseeki64(compress_c, 0, SEEK_SET);
		//on se place au bonne endroit pour la destination
		_fseeki64(fst, dst_cdptr->startPosition + dst_cdptr->totalSize, SEEK_SET);//on se met la ou on doit ajouter le block encodé

		//copy de tmpfile->dst(fst)
		int i;
		for (i = 0; i < size - BLOCK_BUFF_SIZE; i += BLOCK_BUFF_SIZE) {
			fread(block_buff, 1, BLOCK_BUFF_SIZE, compress_c);
			fwrite(block_buff, 1, BLOCK_BUFF_SIZE, fst);
		}
		fread(block_buff, 1, size - i-1, compress_c);
		fwrite(block_buff, 1, size - i-1, fst);

		//fermture du fichier temporaire
		fclose(compress_c);

		cdHeader_BlockMarker_F(fst);//marker de fin de block
		//ajout des données du block dans le header du parents
		cdHeader_addBlockSize(dst_cdptr, start_pos, size + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);//la taille à changé puisqu'on encode 
		
	}
	else
	{

		if (force_copy)//on force la copy
		{

			cdHeader_goStartIndex_F(src_cdptr, fst);//b.GoStartIndex(bl_st);
			//utilise un fichier temporaire
			FILE* childcopy;
			tmpfile_s(&childcopy);
			int64_t  size = src_cdptr->totalSize;
			int64_t  dst_pos = cdHeader_BlockEnd(dst_cdptr);
			int i;
			//copy fst->childcopy
			for (i = 0; i <( size - BLOCK_BUFF_SIZE); i += BLOCK_BUFF_SIZE) {
				fread(block_buff, 1, BLOCK_BUFF_SIZE, fst);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, childcopy);
			}
			fread(block_buff, 1, size - i, fst);
			fwrite(block_buff, 1, size - i, childcopy);
			printf_if_verbose("size childcopy=%lli => size=%lli \n", _ftelli64(childcopy),size);
			//fin de la copie

			//copy chidcopy->fst
			_fseeki64(fst, dst_pos, SEEK_SET);
			_fseeki64(childcopy, 0, SEEK_SET);

			for (i = 0; i <( size - BLOCK_BUFF_SIZE); i += BLOCK_BUFF_SIZE) {
				fread(block_buff, 1, BLOCK_BUFF_SIZE, childcopy);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, fst);
			}
			fread(block_buff, 1, size -i, childcopy);
			fwrite(block_buff, 1, size - i, fst);
			fflush(fst);
			//fin de la copie

			fclose(childcopy);
			printf_if_verbose("copie d'un bloc en %lli de taille %lli fin en %lli \n", dst_pos, size,_ftelli64(fst));
			//cdHeader_BlockMarker_F(fst);// pas marker de début de block
			cdHeader_addBlockSize(dst_cdptr, dst_pos, src_cdptr->totalSize, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
		}
		else//pas de copy 
		{
			//le block est déja dans le fichier et au bonne endroit
			//on n'ajoute que les information
			cdHeader_addBlockSize(dst_cdptr, src_cdptr->startPosition, src_cdptr->totalSize , cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);//taille est inchangé$
			//et on se place à la fin
			_fseeki64(fst, src_cdptr->startPosition + src_cdptr->totalSize, SEEK_SET);//on se met à la fin
			//cdHeader_BlockMarker_F(fst); pas de marker

			printf_if_verbose("arbre (d=%i) contenant les frames %lli à  %lli laissé à la position : %lli de taile %lli\n", depth, src_cdptr->firstFrame,src_cdptr->lastFrame, dst_cdptr->lastAddedBlockPos, src_cdptr->totalSize);
		}


	}
	(*blockCount)++;//on a un block de plus
}



//---------------------------------------------------------------
//version avec Pointer en entré
//-------------------------------------------------------------
//ajout d'une Frame dans un arbre
int cdBlock_addFrameTree_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE* fst, void* frame, int64_t frame_size)
{
	
	//appelle récursif
	if (cdBlock_addFrame_P(cdbptr->child, cdtptr, fst, frame, frame_size)) {
		//propapagation de l'importance
		cdBlock_propagateImportant(cdbptr);
		//ajout de l'enfant terminé aux données du parents
		cdBlock_addChildBlock_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, fst, cdbptr->depth, &cdbptr->blockCount, 0);
		//vérification que le prédicteur fonctionne ou que le nombre de block est atteint
		if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount) && cdHeader_PredictHit_F(cdbptr->header_ptr, fst)) {//pas de problème on peut faire une nouvelle branche
			//il fonctionne
			//on supprime l'enfant actuelle
			cdBlock_DeleteBlock(&cdbptr->child);
			//on le remplace => création récursive
			cdbptr->child = cdBlock_CreateNewChild_F(fst, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1, 0);//CreateNewChild(FileSource);
			printf_if_verbose("on continue l'abre de niveau d=%i\n", cdbptr->depth);
			//on met à jour le header
			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);

			return 0;//ça a fonctionné les parent n'ont rien à faire
		}
		else {
			//echec
			//on termine le parent
			cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
			//mise en valeur des érreur du prédicteur
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf_if_verbose("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n", cdbptr->depth, cdbptr->header_ptr->totalSize, cdbptr->header_ptr->firstFrame, cdbptr->header_ptr->lastFrame);
			}
			printf_if_verbose("abre terminé \n");
			return 1;//on passe le problème à l'abre au dessus;
		}
	}
	else {
		return 0;
	}
}
int cdBlock_addFrameLeaf_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, char* frame, int64_t frame_size)
{

	int64_t  sp = cdbptr->header_ptr->startPosition + cdbptr->header_ptr->totalSize;
	int64_t size = frame_size;
	_fseeki64(fst, sp, SEEK_SET);
	//as ton besoin de compressé?
	if (cdTop_EncodingNeeded(cdtptr, 0))
	{
		//compression
		cdtptr->Encode_PF(frame, fst, frame_size, &size);
		cdHeader_BlockMarker_F(fst);//marker de fin de block
		//ajout des données de la frame au header du block directement
		cdHeader_addBlockSize(cdbptr->header_ptr, sp, size + 1, 1, -1);
	}
	else
	{
		//operation appelé à chaque ajout de frame
		if (cdtptr->per_frame_operation_P(fst, cdbptr->header_ptr, cdtptr, frame, &size)) {
			//pas de copie et addBlockSize doit être appelé
		}
		else {//fonctionement classsique
			printf_if_verbose("frame de taille : %lli\n", frame_size);
			fwrite(frame, 1,frame_size, fst);//écriture des données
			
		}
		int64_t marker_pos = _ftelli64(fst);
		cdHeader_BlockMarker_F(fst);//marker de fin de block
		fflush(fst);
		printf_if_verbose("frame copie a la position : %lli et marker en : %lli copie de %lli\n", sp, marker_pos, size);
		cdHeader_addBlockSize(cdbptr->header_ptr, sp, size + 1, 1, -1);
	}
	cdbptr->blockCount++;//ajout d'un block
	//on vérifie le fonctionnement du prédicteur et que l'on a pas atteint la limite du nombre de block
	if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr, 0, cdbptr->blockCount))
	{
		//mise à jour du header
		cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
		return 0;//pas de problème
	}
	else
	{
		cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
		return 1;//on a un problème
	}

}
//ajout spécial de frame pour les fichiers extern
int cdBlock_addFrameTreeFile_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, char* frame,int64_t frame_size)
{
	if (cdbptr->nodeFile == NULL) {
		cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
		
		CoreDumpBlock* temp = cdBlock_CreateNewChild_F(cdbptr->nodeFile, cdbptr->child->header_ptr->firstFrame, cdbptr->depth - 1, 0);
		cdBlock_DeleteBlock(&cdbptr->child);
		cdbptr->child = temp;
		cdHeader_UpdateHeader(cdbptr->child->header_ptr,cdbptr->nodeFile);
	}
	if (cdBlock_addFrame_P(cdbptr->child, cdtptr, cdbptr->nodeFile, frame,frame_size)) {
		cdBlock_addChildBlockFile_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, &cdbptr->nodeFile, cdbptr->nodeFileName, cdbptr->depth, &cdbptr->blockCount, 0);
		cdBlock_propagateImportant(cdbptr);
		if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))//pas de problème on peut faire une nouvelle branche
		{
			
			cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
			cdBlock_DeleteBlock(&cdbptr->child);
			cdbptr->child = cdBlock_CreateNewChild_F(cdbptr->nodeFile, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1, 0);//CreateNewChild(FileSource);
			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
			return 0;
		}
		else//le predicteur à échouer il faut arrêter
		{
			cdHeader_TerminateBlock(cdbptr->header_ptr, fst);//on met le Block comme terminé (update aussi le header
			cdBlock_CleanChildFile_F(cdbptr);
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf_if_verbose("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n", cdbptr->depth, cdbptr->header_ptr->totalSize, cdbptr->header_ptr->firstFrame, cdbptr->header_ptr->lastFrame);
			}
			printf_if_verbose("abre terminé \n");
			return 1;//le problème doit remonter à l'arbre au dessus
		}
	}
	else
	{
		return 0;
	}

}



//fonction récursive qui gère les différent type de noeud:
//noeud arbre (branches), noeud feuille, noeud arbre avec fichier externe
//cdbptr: block auquel on ajoute une frame
//cdtptr : gestionnaire de module
//fst : fichier dans lequel on ajout le block
//frame : pointeur sur les données à ajouté
//frame_size : taille des données
int cdBlock_addFrame_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, char* frame,int64_t frame_size)
{
	//premier cas : fichier externe
	if (cdHeader_isExternFile(cdbptr->header_ptr) || cdTop_SeparateFileNeeded(cdtptr, cdbptr->depth)) {
		//on met le marker sur le header
		cdHeader_SetExternFile(cdbptr->header_ptr);
		//on met à jour le header
		cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
		//on appelle la fonction associé (cette fonction va probablement rappelé la fonction actuelle => récursivité)
		int v = cdBlock_addFrameTreeFile_P(cdbptr, cdtptr, fst, frame,frame_size);
		return v;
	}
	else {
		//2eme cas : feuille (BASE)
		if (cdHeader_isBaseBlock(cdbptr->header_ptr))
		{
			return cdBlock_addFrameLeaf_P(cdbptr, cdtptr, fst, frame,frame_size);
		}
		else//3eme cas : branches
		{
			
			int v = cdBlock_addFrameTree_P(cdbptr, cdtptr, fst, frame,frame_size);
			return v;
		}
	}
}

//-------------------------------------------------------------------------------------
//operations spéciales
//-------------------------------------------------------------------------------------

//fonction qui force la fin d'un arbre en terminant tous les block en cours
/*
           x
      /         \
     o           x
   /   \       /   \
  o     o     o     x
 / \   / \   / \   / \
o   o o   o o   o o   x
(ici) on termine tous les block en "x" qui étaient en cours (les "o" étaient fini)
*/
void cdBlock_FinishTree_F(CoreDumpBlock* cdbptr,CoreDumpTop* cdtptr,FILE* fst)
{
	if (cdbptr->child != NULL) {//si il y effectivement un enfant
		//si le block utilise un fichier externe
		if (cdHeader_isExternFile(cdbptr->header_ptr)) {
			//on appelle récursivment mais avec ce fichier externe
			cdBlock_FinishTree_F(cdbptr->child, cdtptr, cdbptr->nodeFile);
			//on valide les information du block enfant contenu dans un fichier externe
			cdBlock_addChildBlockFile_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, &cdbptr->nodeFile, cdbptr->nodeFileName, cdbptr->depth, &cdbptr->blockCount, 0);
			//on ferme le ficier externe
			cdBlock_CleanChildFile_F(cdbptr);
		}
		else {
			//on appelle récursivment la fonction pour le niveau inférieur
			cdBlock_FinishTree_F(cdbptr->child, cdtptr, fst);
			//on valide les information du block enfant
			cdBlock_addChildBlock_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr,  fst, cdbptr->depth, &cdbptr->blockCount, 0);
		}
	}
	//on termine le block actuel
	cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
}


//fonction qui écris le nom du fichier associé à cdbptr dans le fichier

void cdBlock_WriteFileName_F(CoreDumpBlock* cdbptr, FILE* fst) 
{
	//on se met la ou on doit écrire le fichier actuelle
	cdHeader_goBlockEnd_F(cdbptr->header_ptr,fst);
	//on l'écris
	fwrite(cdbptr->nodeFileName, 1, strlen(cdbptr->nodeFileName), fst);
	//et on met un marker
	cdHeader_BlockMarker_F(fst);
	fflush(fst);//flush du tampon de fichier
}

