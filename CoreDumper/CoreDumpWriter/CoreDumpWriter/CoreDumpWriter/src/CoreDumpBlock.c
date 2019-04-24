#include "CoreDumpUtils.h"
#include "CoreDumpHeader.h"
#include "CoreDumpType.h"
#include "CoreDumpBlock.h"
#include "CoreDumpTop.h"
#include "CoreDumpConfig.h"
#include <stdlib.h>
#include<string.h>
#include <Windows.h>
#include <time.h>

char block_buff[BLOCK_BUFF_SIZE];
char* string_buff[255];

//--------------------------------------------------------------------------
//Fonction de cr�ation
//---------------------------------------------------------------------------
CoreDumpBlock* cdBlock_Create(FILE* fst, int64_t  first_frame, int no_write) {
	CoreDumpBlock* cdb = (CoreDumpBlock*)malloc(sizeof(CoreDumpBlock));
	cdb->child = NULL;
	cdb->nodeFile = NULL;
	cdb->nodeFileName = NULL;
	cdb->blockCount = 0;
	cdb->header_ptr = cdHeader_Create(_ftelli64(fst));
	cdb->header_ptr->firstFrame = first_frame;
	cdb->depth = 0;
	if (!no_write)cdHeader_WriteHeader_F(cdb->header_ptr, fst);
	return cdb;
}

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
	if (!no_write)cdHeader_WriteHeader_F(cdb->header_ptr, fst);
	return cdb;
}
CoreDumpBlock* cdBlock_CreateTree_F(FILE* fst,int64_t  first_frame,int depth_a,int no_write)
{
	CoreDumpBlock* cdb = (CoreDumpBlock*)malloc(sizeof(CoreDumpBlock));
	cdb->blockCount = 0;
	cdb->header_ptr = cdHeader_Create(_ftelli64(fst));
	cdb->header_ptr->firstFrame = first_frame;
	cdb->depth = depth_a;
	cdb->child = NULL;
	cdb->nodeFile = NULL;
	cdb->nodeFileName = NULL;
	if (!no_write) 
	{
		cdHeader_WriteHeader_F(cdb->header_ptr, fst);
		cdb->child = cdBlock_CreateNewChild_F(fst, first_frame, depth_a-1,0);
	}
	return cdb;
}
void cdBlock_CreateNewChildFile_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE* fst)
{
	if (cdbptr->nodeFile != NULL) {
		fclose(cdbptr->nodeFile);
		
	}
	if(cdbptr->nodeFileName!=NULL)free(cdbptr->nodeFileName);//pas de fuite

	cdbptr->nodeFileName =cdTop_SeparateFileName(cdtptr,cdbptr->depth, cdbptr->blockCount);
	remove(cdbptr->nodeFileName);//supression du fichier si il existe d�ja
	fopen_s(&cdbptr->nodeFile, cdbptr->nodeFileName, "wb+");
	//Console.WriteLine("nouveau fichier cr�e :" + CurrentNodeFileName);
	printf("nouveau fichier creer : %s\n", cdbptr->nodeFileName);
	//on inscris dans le fst le nom du fichier
	_fseeki64(cdbptr->nodeFile, 0, SEEK_SET);
	cdBlock_WriteFileName_F(cdbptr, fst);
}
CoreDumpBlock* cdBlock_CreateNewChild_F(FILE* fst, int64_t  first_frame, int depth_a,int no_write)
{
	printf("Creation  d'enfant : premiere frame %lli depth=%i en %lli avec no_write=%i\n" , first_frame, depth_a, _ftelli64(fst),no_write);
	if (depth_a > 0) {
		return cdBlock_CreateTree_F(fst, first_frame, depth_a,no_write);
	}
	else 
	{
		return cdBlock_CreateLeaf_F(fst, first_frame,no_write);
	}
}
void cdBlock_DeleteBlock(CoreDumpBlock* cdbptr) {
	if (cdbptr->child != NULL)cdBlock_DeleteBlock(cdbptr->child);
	cdHeader_Delete(cdbptr->header_ptr);
	if(cdbptr->nodeFileName!=NULL)free(cdbptr->nodeFileName);
	if(cdbptr->nodeFile!=NULL)fclose(cdbptr->nodeFile);
	free(cdbptr);
}
//--------------------------------------------
//--threaded Encode
//--------------------------------------------
struct dataThreadEncode
{
	CoreDumpTop* cdtptr;
	FILE* input;
	FILE* output;
	int64_t inSize;
	int64_t* outSize;
	char* input_name;
};
#ifdef _WIN32
DWORD WINAPI ThreadProc_Encode(LPVOID lpParameter) {
	struct dataThreadEncode* param = lpParameter;
	cdTop_IncSema(param->cdtptr);
	param->cdtptr->Encode_FF(param->input, param->output, param->inSize, param->outSize);
	cdTop_ReleaseSema(param->cdtptr);
	fclose(param->input);
	fclose(param->output);
	remove(param->input_name);
	free(param->input_name);
	free(param);
}
#endif // _WIN32




void cdBlock_ThreadedEncode_FF(CoreDumpTop* cdtptr,char* input_name,FILE** input, FILE** output, int64_t inSize, int64_t* outSize) 
{
	struct dataThreadEncode* param=malloc(sizeof(struct dataThreadEncode));
	param->cdtptr = cdtptr;
	param->input = *input;
	param->output = *output;
	param->inSize = inSize;
	param->outSize = outSize;
	param->input_name = input_name;
	printf("Thread de compression lanc�\n");
	#ifdef _WIN32
		CreateThread(NULL, NULL, ThreadProc_Encode, param, NULL,NULL);
	#else
		#error "Pthread pas impl�mente"
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
	/*if (!CurrentNodeFile.CanWrite)
	{
		CreateNewChildFile();
	}*/
	// /!\ fst have to exist => fait attentioon
	if (cdTop_EncodingNeeded(cdtptr, depth) || cdHeader_IsCompressed(dst_cdptr))//on doit encoder ?
	{
		cdHeader_SetCompressed(dst_cdptr);

		fclose(*node_fst);
		char* temp_node_name = malloc(strlen(nodeFileName) + 10);
		strcpy(temp_node_name, nodeFileName);
		strcat(temp_node_name, ".backup");
		remove(temp_node_name);
		rename(nodeFileName, temp_node_name);
		FILE* temp_node;
		fopen_s(&temp_node, temp_node_name, "rb");
		fopen_s(node_fst, nodeFileName, "wb");//safe open=> node_fst is a ptr to FILE*
		if (!*node_fst) {
			fprintf(stderr, "error opening %s: %s", nodeFileName, strerror(errno));
		}
		//_fseeki64(0, temp_node,SEEK_SET);
		int64_t  size;

		struct timespec before,after;
		timespec_get(&before, TIME_UTC);
		#ifndef USE_THREADED_ENCODE
			printf("Compression en cours \n");
			cdtptr->Encode_FF(temp_node, *node_fst, src_cdptr->totalSize, &size);
			timespec_get(&after, TIME_UTC);
			printf("Compression de %llu a %llu en %s\n", src_cdptr->totalSize, size,get_time_diff(string_buff,before,after));
		#else
			cdBlock_ThreadedEncode_FF(cdtptr, temp_node_name, &temp_node, node_fst, src_cdptr->totalSize, &size);
		#endif
		int64_t  start_pos = dst_cdptr->startPosition + dst_cdptr->totalSize;

		cdHeader_addBlockSize(dst_cdptr, start_pos, strlen(nodeFileName) + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
		#ifndef USE_THREADED_ENCODE
			fclose(temp_node);
			remove(temp_node_name);
			free(temp_node_name);
		#endif	
		//addBlockSize(start_pos, compress_size, b.FrameInBlock, b.FirstFrame);//la taille � chang� puisqu'on encode
		printf("arbre (d=%i) contenant les frames %lli �  %lli encod� � la position : %lli r�duit de %lli � %lli du fichier %s\n", depth, src_cdptr->firstFrame,src_cdptr->lastFrame ,src_cdptr->lastAddedBlockPos, src_cdptr->totalSize, size,nodeFileName);
	//Console.WriteLine("arbre (d=" + Depth + ") contenant les frames " + b.FirstFrame + " � " + b.LastFrame + " encod� � la position :" + LastAddedBlockPos.ToString() + "r�duit de " + b.TotalSize + " � " + compress_size);

	}
	else
	{
		//le block est d�ja dans le fichier
		//FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encod�
		int64_t  start_pos = dst_cdptr->startPosition + dst_cdptr->totalSize;

		//FileSource.Write(Encoding.ASCII.GetBytes(CurrentNodeFileName), 0, CurrentNodeFileName.Length);//ce n'est pas les nom des fichier qui sont encod� mais  le ficgier (mais �a c'est d�ja fait)
		//BlockMarker(FileSource);

		//int64_t  compress_size = (FileSource.Position - start_pos);
		cdHeader_addBlockSize(dst_cdptr, start_pos, strlen(nodeFileName) + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
		//addBlockSize(start_pos, compress_size, b.FrameInBlock, b.FirstFrame);//la taille � chang� puisqu'on encode 

		//Console.WriteLine("arbre (d=" + Depth + ") contenant " + b.FirstFrame + " � " + b.LastFrame + "  laiss� � la position :" + LastAddedBlockPos.ToString() + "de taille" + b.TotalSize);



	}
	*blockCount++;//on a un block de plus
}
void cdBlock_addChildBlock_F(coreDumpHeader* src_cdptr, coreDumpHeader* dst_cdptr, CoreDumpTop* cdtptr, FILE* fst, int depth, int* blockCount, int force_copy)
{
	if (cdTop_EncodingNeeded(cdtptr,depth) || cdHeader_IsCompressed(dst_cdptr))//on doit encoder ?
	{
		cdHeader_SetCompressed(dst_cdptr);

		cdHeader_goStartIndex_F(src_cdptr, fst);//b.GoStartIndex(bl_st);
		int64_t  start_pos = _ftelli64(fst);
#ifdef NO_TEMP_FILE
#else
		FILE* compress_c;
		tmpfile_s(&compress_c);
		int64_t  size;
		cdtptr->Encode_FF(fst, compress_c, src_cdptr->totalSize, &size);
		_fseeki64(compress_c, 0, SEEK_SET);
		_fseeki64(fst, dst_cdptr->startPosition + dst_cdptr->totalSize, SEEK_SET);//on se met la ou on doit ajouter le block encod�

		int i;
		for (i = 0; i < size - BLOCK_BUFF_SIZE; i += BLOCK_BUFF_SIZE) {
			fread(block_buff, 1, BLOCK_BUFF_SIZE, compress_c);
			fwrite(block_buff, 1, BLOCK_BUFF_SIZE, fst);
		}
		fread(block_buff, 1, size - i-1, compress_c);
		fwrite(block_buff, 1, size - i-1, fst);

		fclose(compress_c);
		cdHeader_BlockMarker_F(fst);
		cdHeader_addBlockSize(dst_cdptr, start_pos, size + 1, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);//la taille � chang� puisqu'on encode 
		//Console.WriteLine("arbre (d=" + Depth + ") contenant les frames " + b.FirstFrame + " � " + b.LastFrame + " encod� � la position :" + LastAddedBlockPos.ToString() + "r�duit de " + b.TotalSize + " � " + compress_size);

#endif // NO_TEMP_FILE


	}
	else
	{

		if (force_copy)
		{

			cdHeader_goStartIndex_F(src_cdptr, fst);//b.GoStartIndex(bl_st);
#ifdef NO_TEMP_FILE


#else
			FILE* childcopy;
			tmpfile_s(&childcopy);
			int64_t  size = src_cdptr->totalSize;
			//int64_t  src_pos = src_cdptr->startPosition;
			int64_t  dst_pos = dst_cdptr->startPosition + dst_cdptr->totalSize;
			int i;
			//copy fst->childcopy
			for (i = 0; i <( size - BLOCK_BUFF_SIZE); i += BLOCK_BUFF_SIZE) {
				fread(block_buff, 1, BLOCK_BUFF_SIZE, fst);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, childcopy);
			}
			fread(block_buff, 1, size - i, fst);
			fwrite(block_buff, 1, size - i, childcopy);
			printf("size childcopy=%lli\n", _ftelli64(childcopy));
			//copy chidcopy->fst
			_fseeki64(fst, dst_pos, SEEK_SET);
			_fseeki64(childcopy, 0, SEEK_SET);

			for (i = 0; i <( size - BLOCK_BUFF_SIZE); i += BLOCK_BUFF_SIZE) {
				fread(block_buff, 1, BLOCK_BUFF_SIZE, childcopy);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, fst);
			}
			fread(block_buff, 1, size -i, childcopy);
			fwrite(block_buff, 1, size - i, fst);
			printf("copie d'un bloc en %lli de taille %lli fin en %lli \n", dst_pos, size,_ftelli64(fst));
			//cdHeader_BlockMarker_F(fst);//marker de d�but de block
			cdHeader_addBlockSize(dst_cdptr, dst_pos, src_cdptr->totalSize, cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);
#endif
		}
		else
		{
			//le block est d�ja dans le fichier
			cdHeader_addBlockSize(dst_cdptr, src_cdptr->startPosition, src_cdptr->totalSize , cdHeader_FrameInBlock(src_cdptr), src_cdptr->firstFrame);//taille est inchang�$
			_fseeki64(fst, src_cdptr->startPosition + src_cdptr->totalSize, SEEK_SET);//on se met � la fin
			//cdHeader_BlockMarker_F(fst);

			printf("arbre (d=%i) contenant les frames %lli �  %lli laiss� � la position : %lli de taile %lli\n", depth, src_cdptr->firstFrame,src_cdptr->lastFrame, dst_cdptr->lastAddedBlockPos, src_cdptr->totalSize);
			//Console.WriteLine("arbre (d=" + Depth + ") contenant " + b.FirstFrame + " � " + b.LastFrame + "  laiss� � la position :" + LastAddedBlockPos.ToString() + "de taille" + b.TotalSize);
		}


	}
	(*blockCount)++;//on a un block de plus
}



int cdBlock_addFrameTree_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE* fst,FILE* frame) 
{
	if (cdBlock_addFrame_F(cdbptr->child, cdtptr, fst,frame)) {

		cdBlock_addChildBlock_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, fst, cdbptr->depth, &cdbptr->blockCount,0);
		if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr,cdbptr->depth, cdbptr->blockCount)) {//pas de probl�me on peut faire une nouvelle branche
			printf("on continue l'abre de niveau d=%i", cdbptr->depth);

			cdBlock_DeleteBlock(cdbptr->child);
			cdbptr->child = cdBlock_CreateNewChild_F(fst, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1,0);//CreateNewChild(FileSource);

			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);

			return 0;
		}
		else {
			cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n", cdbptr->depth, cdbptr->header_ptr->totalSize, cdbptr->header_ptr->firstFrame, cdbptr->header_ptr->lastFrame);
			}
			printf("abre termin� \n");
			return 1;//on passe le probl�me � l'abre au dessus;
		}
	}
	else {
		return 0;
	}
}
int cdBlock_addFrameLeaf_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst,FILE* frame) {
	int64_t  sp =cdbptr->header_ptr->startPosition +cdbptr->header_ptr->totalSize;
	//System.Console.WriteLine("block �cris en " + sp.ToString());
	if (cdTop_EncodingNeeded(cdtptr,0))
	{
		_fseeki64(fst, sp, SEEK_SET);
		int64_t  size;
		cdtptr->Encode_FF(frame, fst, -1, &size);
		///Top.Encode(st, FileSource, -1, -1);
		cdHeader_BlockMarker_F(fst);//marker de fin de block
		cdHeader_addBlockSize(cdbptr->header_ptr,sp, size + 1, 1,-1);
	}
	else
	{
		_fseeki64(fst, sp, SEEK_SET);
		_fseeki64(frame, 0, SEEK_END);
		int64_t  end_pos = _ftelli64(frame);
		_fseeki64(frame, 0, SEEK_SET);
		int64_t  start_pos = _ftelli64(frame);
		int64_t  size = end_pos - start_pos;
		if (cdtptr->per_frame_operation_F(fst, cdbptr->header_ptr, cdtptr, frame, &size)) {
			//pas de copie et addBlockSize doit �tre appel�
		}
		else {
			printf("frame de taille : %lli\n", size);
			//FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encod�
			int i;
			int read_size = 0;
			int64_t test3 = _ftelli64(fst);
			for (i = 0; (i + BLOCK_BUFF_SIZE) < (size); i += BLOCK_BUFF_SIZE) {
				read_size += fread(block_buff, 1LL, BLOCK_BUFF_SIZE, frame);
				fwrite(block_buff, 1, BLOCK_BUFF_SIZE, fst);
			}
			int64_t test = _ftelli64(fst); int64_t test2 = i + sp;
			fread(block_buff, 1, size - i, frame);
			fwrite(block_buff, 1, size - i, fst);
			int64_t marker_pos = _ftelli64(fst);
			//st.CopyTo(FileSource);
			//BlockMarker(FileSource);//marker de fin de block

			cdHeader_BlockMarker_F(fst);//marker de fin de block
			//addBlockSize(sp, st.Length + 1, 1);
			printf("frame copie a la position : %lli et marker en : %lli copie de %lli\n", sp, marker_pos, size);
			cdHeader_addBlockSize(cdbptr->header_ptr, sp, size + 1, 1, -1);
		}
	}
	cdbptr->blockCount++;
	if(cdHeader_PredictHit_F(cdbptr->header_ptr,fst)&&!cdTop_MaxBlockCountReach(cdtptr,0,cdbptr->blockCount))
	{
		printf("On continue � rajouter des block � la feuille \n");
		cdHeader_UpdateHeader(cdbptr->header_ptr,fst);
		return 0;//pas de probl�me
	}
	else
	{
		printf("On a fini une feuille");
		cdHeader_TerminateBlock(cdbptr->header_ptr,fst);
		/*if (!Top.MaxBlockCountReach(0, blockCount))
		{
			Console.WriteLine("echec predicteur d=" + Depth + ", on fini avec Totalize=" + TotalSize + "(entre " + FirstFrame + " et " + LastFrame + ")");
		}*/
		return 1;//on a un probl�me
	}

}
int cdBlock_addFrameTreeFile_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, FILE* frame) 
{
	if (cdbptr->nodeFile == NULL) {
		cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
	}
	if (cdBlock_addFrame_F(cdbptr->child, cdtptr, cdbptr->nodeFile, frame)) {
		cdBlock_addChildBlockFile_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, &cdbptr->nodeFile, cdbptr->nodeFileName, cdbptr->depth, &cdbptr->blockCount, 0);
		if(cdHeader_PredictHit_F(cdbptr->header_ptr,fst)&& !cdTop_MaxBlockCountReach(cdtptr,cdbptr->depth, cdbptr->blockCount))//pas de probl�me on peut faire une nouvelle branche
		{

			cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
			cdBlock_DeleteBlock(cdbptr->child);
			cdbptr->child = cdBlock_CreateNewChild_F(cdbptr->nodeFile, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1,0);//CreateNewChild(FileSource);
			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
			return 0;
		}
		else//le predicteur � �chouer il faut arr�ter
		{
			cdHeader_TerminateBlock(cdbptr->header_ptr,fst);//on met le Block comme termin� (update aussi le header
			if(cdbptr->nodeFile !=NULL)fclose(cdbptr->nodeFile);
			cdbptr->nodeFile = NULL;
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n",cdbptr->depth,cdbptr->header_ptr->totalSize,cdbptr->header_ptr->firstFrame,cdbptr->header_ptr->lastFrame);
			}
			printf("abre termin� \n");
			return 1;//le probl�me doit remonter � l'arbre au dessus
		}
	}
	else 
	{
		return 0;
	}

}



int cdBlock_addFrame_F(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst,FILE* frame)
{
	if (cdHeader_isExternFile(cdbptr->header_ptr) || cdTop_SeparateFileNeeded(cdtptr,cdbptr->depth)) {
		cdHeader_SetExternFile(cdbptr->header_ptr);
		cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
		return cdBlock_addFrameTreeFile_F(cdbptr, cdtptr, fst, frame);
	}
	else {
		if (cdHeader_isBaseBlock(cdbptr->header_ptr))
		{
			return cdBlock_addFrameLeaf_F(cdbptr, cdtptr, fst, frame);
		}
		else
		{
			return cdBlock_addFrameTree_F(cdbptr, cdtptr, fst, frame);
		}
	}
}
//---------------------------------------------------------------
//version avec Pointer en entr�
//-------------------------------------------------------------
int cdBlock_addFrameTree_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE* fst, FILE* frame, int64_t frame_size)
{
	if (cdBlock_addFrame_P(cdbptr->child, cdtptr, fst, frame, frame_size)) {

		cdBlock_addChildBlock_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, fst, cdbptr->depth, &cdbptr->blockCount, 0);
		if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount)) {//pas de probl�me on peut faire une nouvelle branche
			cdBlock_DeleteBlock(cdbptr->child);
			cdbptr->child = cdBlock_CreateNewChild_F(fst, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1, 0);//CreateNewChild(FileSource);
			printf("on continue l'abre de niveau d=%i\n", cdbptr->depth);
			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);

			return 0;
		}
		else {
			cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n", cdbptr->depth, cdbptr->header_ptr->totalSize, cdbptr->header_ptr->firstFrame, cdbptr->header_ptr->lastFrame);
			}
			printf("abre termin� \n");
			return 1;//on passe le probl�me � l'abre au dessus;
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
	if (cdTop_EncodingNeeded(cdtptr, 0))
	{
		cdtptr->Encode_PF(frame, fst, frame_size, &size);
		cdHeader_BlockMarker_F(fst);//marker de fin de block
		cdHeader_addBlockSize(cdbptr->header_ptr, sp, size + 1, 1, -1);
	}
	else
	{
		
		if (cdtptr->per_frame_operation_P(fst, cdbptr->header_ptr, cdtptr, frame, &size)) {
			//pas de copie et addBlockSize doit �tre appel�
		}
		else {//fonctionement classsique
			
			printf("frame de taille : %lli\n", frame_size);
			//FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encod�
			
			fwrite(frame, 1,frame_size, fst);
			
			//st.CopyTo(FileSource);
			//BlockMarker(FileSource);//marker de fin de block
			
			//addBlockSize(sp, st.Length + 1, 1);
			
		}
		int64_t marker_pos = _ftelli64(fst);
		cdHeader_BlockMarker_F(fst);//marker de fin de block
		printf("frame copie a la position : %lli et marker en : %lli copie de %lli\n", sp, marker_pos, size);
		cdHeader_addBlockSize(cdbptr->header_ptr, sp, size + 1, 1, -1);
	}
	cdbptr->blockCount++;
	if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr, 0, cdbptr->blockCount))
	{
		cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
		return 0;//pas de probl�me
	}
	else
	{
		cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
		/*if (!Top.MaxBlockCountReach(0, blockCount))
		{
			Console.WriteLine("echec predicteur d=" + Depth + ", on fini avec Totalize=" + TotalSize + "(entre " + FirstFrame + " et " + LastFrame + ")");
		}*/
		return 1;//on a un probl�me
	}

}
int cdBlock_addFrameTreeFile_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, char* frame,int64_t frame_size)
{
	if (cdbptr->nodeFile == NULL) {
		cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
		
		CoreDumpBlock* temp = cdBlock_CreateNewChild_F(cdbptr->nodeFile, cdbptr->child->header_ptr->firstFrame, cdbptr->depth - 1, 0);
		cdBlock_DeleteBlock(cdbptr->child);
		cdbptr->child = temp;
		cdHeader_UpdateHeader(cdbptr->child->header_ptr,cdbptr->nodeFile);
	}
	if (cdBlock_addFrame_P(cdbptr->child, cdtptr, cdbptr->nodeFile, frame,frame_size)) {
		cdBlock_addChildBlockFile_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, &cdbptr->nodeFile, cdbptr->nodeFileName, cdbptr->depth, &cdbptr->blockCount, 0);
		if (cdHeader_PredictHit_F(cdbptr->header_ptr, fst) && !cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))//pas de probl�me on peut faire une nouvelle branche
		{

			cdBlock_CreateNewChildFile_F(cdbptr, cdtptr, fst);
			cdBlock_DeleteBlock(cdbptr->child);
			cdbptr->child = cdBlock_CreateNewChild_F(cdbptr->nodeFile, cdbptr->header_ptr->lastFrame, cdbptr->depth - 1, 0);//CreateNewChild(FileSource);
			cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
			return 0;
		}
		else//le predicteur � �chouer il faut arr�ter
		{
			cdHeader_TerminateBlock(cdbptr->header_ptr, fst);//on met le Block comme termin� (update aussi le header
			if (cdbptr->nodeFile != NULL)fclose(cdbptr->nodeFile);
			cdbptr->nodeFile = NULL;
			if (!cdTop_MaxBlockCountReach(cdtptr, cdbptr->depth, cdbptr->blockCount))
			{
				printf("echec predicteur d=%i, on fini avec Totalize= %lli (entre %lli et %lli )\n", cdbptr->depth, cdbptr->header_ptr->totalSize, cdbptr->header_ptr->firstFrame, cdbptr->header_ptr->lastFrame);
			}
			printf("abre termin� \n");
			return 1;//le probl�me doit remonter � l'arbre au dessus
		}
	}
	else
	{
		return 0;
	}

}




int cdBlock_addFrame_P(CoreDumpBlock* cdbptr, CoreDumpTop* cdtptr, FILE*fst, char* frame,int64_t frame_size)
{
	if (cdHeader_isExternFile(cdbptr->header_ptr) || cdTop_SeparateFileNeeded(cdtptr, cdbptr->depth)) {
		cdHeader_SetExternFile(cdbptr->header_ptr);
		cdHeader_UpdateHeader(cdbptr->header_ptr, fst);
		return cdBlock_addFrameTreeFile_P(cdbptr, cdtptr, fst, frame,frame_size);
	}
	else {
		if (cdHeader_isBaseBlock(cdbptr->header_ptr))
		{
			return cdBlock_addFrameLeaf_P(cdbptr, cdtptr, fst, frame,frame_size);
		}
		else
		{
			return cdBlock_addFrameTree_P(cdbptr, cdtptr, fst, frame,frame_size);
		}
	}
}

//-------------------------------------------------------------------------------------
//operations sp�ciales
//-------------------------------------------------------------------------------------


void cdBlock_FinishTree_F(CoreDumpBlock* cdbptr,CoreDumpTop* cdtptr,FILE* fst)
{
	if (cdbptr->child != NULL) {
		cdBlock_FinishTree_F(cdbptr->child, cdtptr, fst);
		if (cdHeader_isExternFile(cdbptr->header_ptr)) {
			cdBlock_addChildBlockFile_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr, &cdbptr->nodeFile, cdbptr->nodeFileName, cdbptr->depth, &cdbptr->blockCount, 0);
			fclose(cdbptr->nodeFile);
			cdbptr->nodeFile = NULL;
		}
		else {
			cdBlock_addChildBlock_F(cdbptr->child->header_ptr, cdbptr->header_ptr, cdtptr,  fst, cdbptr->depth, &cdbptr->blockCount, 0);
		}
	}
	cdHeader_TerminateBlock(cdbptr->header_ptr, fst);
}



void cdBlock_WriteFileName_F(CoreDumpBlock* cdbptr, FILE* fst) 
{
	int64_t  start_pos = cdbptr->header_ptr->startPosition + cdbptr->header_ptr->totalSize;
	_fseeki64(fst, start_pos, SEEK_SET);
	fwrite(cdbptr->nodeFileName, 1, strlen(cdbptr->nodeFileName), fst);
	cdHeader_BlockMarker_F(fst);
}

//TODO => finir la fonction (gestion du cas fichier extern)
int cdBlock_RestoreTree(CoreDumpBlock* cdbptr, FILE* fst) {
	if(cdHeader_isExternFile(cdbptr->header_ptr))cdHeader_BlockEnd(cdbptr);
	CoreDumpBlock* tmp= cdBlock_Create(fst, cdbptr->header_ptr->firstFrame, 1);
	cdHeader_UpdateFromFile(tmp->header_ptr, fst);
	if (cdHeader_isBaseBlock(tmp->header_ptr)) {
		tmp->depth = 0;
		cdbptr->depth = 1;
		cdbptr->child = tmp;
		return 0;
	}
	else {
		if (cdHeader_isExternFile(tmp->header_ptr)) {
			//cdbptr->nodeFilename= //read file name
			//cdbptr->nodeFile=fopen open file
			//pas fini
			cdbptr->depth = cdBlock_RestoreTree(tmp, cdbptr->nodeFile)+1;
		}
		else {
			cdbptr->depth = cdBlock_RestoreTree(tmp, fst)+1;
			
		}
		cdbptr->child = tmp;
		return cdbptr->depth;
	}
}