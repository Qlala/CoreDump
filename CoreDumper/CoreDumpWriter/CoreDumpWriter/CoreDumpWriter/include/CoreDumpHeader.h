

#include <stdio.h>
#include<stdint.h>
#pragma once
enum config_flags { COMPRESSED = 0x1, FINISHED = 0x2, BASE = 0x4, _32BIT_HEADER = 0x8, EXTERN_FILE = 0x10 ,IMPORTANT=0x80};
#define CD_HEADER_SIZE 8*7+1;


//Structure de gestion du Header
struct coreDumpHeader_S {
	int64_t headerSize ;//pas stocké
	int64_t totalSize;//prend en compte le header
    int64_t predBlockSize ;
	int64_t predFramePerBlock ;
	int64_t firstFrame ;//TODO => penser au cas avec peu de noeud dans l'abre : le prédicteur consome beaucoup de donner
	int64_t lastFrame ;//dernière frame ajouté au tableau de block si fini , sinon  c'est la dernière frame du dernier block qui a été ajouté
	int64_t lastAddedBlockPos;//par rapport à start Position
	int64_t firstFrameOfLastBlock;
	unsigned char configuration ;

	//Les éléments manipulés par les header (données ajoutées) sont appelés frames.

	//EXTERN_FILE => the block only contains file string (BLOCK MARK maybe subituted with \0 )
	//
	//Header
	/*
	||configuration(1)|totalSize(8)|predBlockSize(8)|predFramePerBlock(8)|firstFrame(8)|lastFrame(8)|lastAddedBlockPos(8);
	 0      Headersize     TotalSize
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


//Creation et destruction d'un Header
//start_index est la position de création (StartPosition)
coreDumpHeader * cdHeader_Create(int64_t start_index);
void cdHeader_Delete(coreDumpHeader * cdptr);

//Verifie la présenece du propriété dans configuration (COMPRESSED,BASE,EXTERN_FILE,IMPORTANT)
int cdHeader_IsCompressed(coreDumpHeader * cdptr);
int cdHeader_isBaseBlock(coreDumpHeader * cdptr);
int cdHeader_isExternFile(coreDumpHeader * cdptr);
int cdHeader_isImportant(CoreDumpHeader * cdptr);

//active les flag de proprété dans configuration(IMPORTANT, COMPRESSED,EXTERN_FILE)
void cdHeader_setImportant(CoreDumpHeader * cdptr);
void cdHeader_SetCompressed(coreDumpHeader * cdptr);
void cdHeader_SetExternFile(coreDumpHeader * cdptr);

//compte le nombre de frame dans les blocks
int64_t  cdHeader_FrameInBlock(coreDumpHeader * cdptr);

//Place le charriot de fst à la position StartPosition
void cdHeader_goStartIndex_F(coreDumpHeader * cdptr, FILE * fst);
//Place le charriot de fst à la position de fin (StartPosition+Totalsize)
void cdHeader_goBlockEnd_F(coreDumpHeader * cdptr, FILE * fst);

//fini le block et met à jour le header dans le fichier fst.
void cdHeader_TerminateBlock(coreDumpHeader * cdptr, FILE * fst);

//met à jour le header(place automatiquement à la bonne place)
void cdHeader_UpdateHeader(coreDumpHeader * cdptr, FILE * fst);//s
void cdHeader_UpdateHeader_no_restore(coreDumpHeader * cdptr, FILE * fst);//ne remet le fichier à la position qu'il avait avant la mise à jour

//met à jour les donnés de la structure depuis les donnés dans fst (en se basant sur StartPosition)
void cdHeader_UpdateFromFile(coreDumpHeader * cdptr, FILE * fst);

//Lis et écris le header depuis fst =>
//le header est écris et lu depuis la position du chariot de fst
void cdHeader_ReadHeader_F(coreDumpHeader * cdptr, FILE * fst);
void cdHeader_WriteHeader_F(coreDumpHeader * cdptr, FILE * fst);

//donne la position pour la frame f dans le fichier fst
int64_t  cdHeader_PredictFramePosition_F(coreDumpHeader * cdptr, int64_t  f, FILE * fst);
//vérifie le predicteur pour le dernier ajout
int cdHeader_PredictHit_F(coreDumpHeader * cdptr, FILE * fst);

//recherche la fin du block ou le chariot de fst est placé
int64_t  cdHeader_SearchBlockEnd_F(coreDumpHeader * cdptr, FILE * fst);

//ajoute les donnés dans le header
//cdptr : le  header
//Pos :  position du block ajouté
//size : taille du block ajouté
//frameinblock_a : nombre de frame dans le block
//firstFrameofBlock : premier
void cdHeader_addBlockSize(coreDumpHeader * cdptr, int64_t  Pos, int64_t  size, int64_t  frameinblock_a, int64_t  firstFrameOfBlock);

//retourne la position de fin des block
int64_t  cdHeader_BlockEnd(coreDumpHeader * cdptr);

//marque la fin d'un block à l'aide de MARK_CHAR
void cdHeader_BlockMarker_F(FILE * fst);

//affiche les information d'un Block
void cdHeader_printInfo(coreDumpHeader * cdptr);
