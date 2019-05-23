

#include <stdio.h>
#include<stdint.h>
#pragma once
enum config_flags { COMPRESSED = 0x1, FINISHED = 0x2, BASE = 0x4, _32BIT_HEADER = 0x8, EXTERN_FILE = 0x10 ,IMPORTANT=0x80};
#define CD_HEADER_SIZE 8*7+1;
struct coreDumpHeader_S {
	int64_t headerSize ;//pas stocké
	int64_t totalSize;//prend en compte le header
    int64_t predBlockSize ;//absent si NO_PRED
	int64_t predFramePerBlock ;//absent si NO_PRED
	int64_t firstFrame ;//TODO => penser au cas avec peu de noeud dans l'abre : le prédicteur consome beaucoup de donner
	int64_t lastFrame ;//dernière frame ajouté au tableau de block si fini , sinon  c'est la dernière frame du dernier block qui a été ajouté
	int64_t lastAddedBlockPos;//par rapport à start Posotion
	int64_t firstFrameOfLastBlock;
	unsigned char configuration ;

	//EXTERN_FILE => the block only contains file string (BLOCK MARK maybe subituted with \0 )
	//
	//Header
	/*
	||totalSize(8)|predBlockSize(8)|predFramePerBlock(8)|firstFrame(8)|lastFrame(8)|lastAddedBlockPos(8)|configuration(1);
	 0      Hsize          TotalSize
	 |      |              |
	\ /    \ /            \ /
	|Header| |BLOCKS .... | UNFINISHED_BLOCKS |


   */
   //not in the header => deduce by file start position

   //les prédiction et les POS se repère par rapport au début du fichier et non par rapport à start pos
	//non stocké dans le header
	int64_t startPosition;//avant le header;
	int64_t lastAddedBlockSize;
};


typedef struct coreDumpHeader_S coreDumpHeader;
typedef struct coreDumpHeader_S CoreDumpHeader;


coreDumpHeader * cdHeader_Create(int64_t start_index);

void cdHeader_Delete(coreDumpHeader * cdptr);

int cdHeader_IsCompressed(coreDumpHeader * cdptr);

int cdHeader_isBaseBlock(coreDumpHeader * cdptr);

int cdHeader_isExternFile(coreDumpHeader * cdptr);

void cdHeader_setImportant(CoreDumpHeader * cdptr);

int cdHeader_isImportant(CoreDumpHeader * cdptr);

void cdHeader_SetCompressed(coreDumpHeader * cdptr);

void cdHeader_SetExternFile(coreDumpHeader * cdptr);

int64_t  cdHeader_FrameInBlock(coreDumpHeader * cdptr);

void cdHeader_goStartIndex_F(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_TerminateBlock(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_UpdateHeader(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_UpdateHeader_no_restore(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_UpdateFromFile(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_ReadHeader_F(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_WriteHeader_F(coreDumpHeader * cdptr, FILE * fst);

int64_t  cdHeader_PredictFramePosition_F(coreDumpHeader * cdptr, int64_t  f, FILE * fst);

int cdHeader_PredictHit_F(coreDumpHeader * cdptr, FILE * fst);

int64_t  cdHeader_SearchBlockEnd_F(coreDumpHeader * cdptr, FILE * fst);

void cdHeader_addBlockSize(coreDumpHeader * cdptr, int64_t  Pos, int64_t  size, int64_t  frameinblock_a, int64_t  firstFrameOfBlock);

int64_t  cdHeader_BlockEnd(coreDumpHeader * cdptr);

void cdHeader_BlockMarker_F(FILE * fst);

void cdHeader_printInfo(coreDumpHeader * cdptr);
