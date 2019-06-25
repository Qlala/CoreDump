

#include <stdio.h>
#include<stdint.h>
#pragma once
enum config_flags { COMPRESSED = 0x1, FINISHED = 0x2, BASE = 0x4, _32BIT_HEADER = 0x8, EXTERN_FILE = 0x10 ,IMPORTANT=0x80};
#define CD_HEADER_SIZE 8*7+1;


//Structure de gestion du Header
struct coreDumpHeader_S {
	int64_t headerSize ;//pas stock�
	int64_t totalSize;//prend en compte le header
    int64_t predBlockSize ;
	int64_t predFramePerBlock ;
	int64_t firstFrame ;//TODO => penser au cas avec peu de noeud dans l'abre : le pr�dicteur consome beaucoup de donner
	int64_t lastFrame ;//derni�re frame ajout� au tableau de block si fini , sinon  c'est la derni�re frame du dernier block qui a �t� ajout�
	int64_t lastAddedBlockPos;//par rapport � start Position
	int64_t firstFrameOfLastBlock;
	unsigned char configuration ;

	//Les �l�ments manipul�s par les header (donn�es ajout�es) sont appel�s frames.

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

   //les pr�diction et les POS se rep�re par rapport au d�but du fichier et non par rapport � start pos
	//non stock� dans le header
	int64_t startPosition;//avant le header;
	int64_t lastAddedBlockSize;
};


typedef struct coreDumpHeader_S coreDumpHeader;
typedef struct coreDumpHeader_S CoreDumpHeader;


//Creation et destruction d'un Header
//start_index est la position de cr�ation (StartPosition)
coreDumpHeader * cdHeader_Create(int64_t start_index);
void cdHeader_Delete(coreDumpHeader * cdptr);

//Verifie la pr�senece du propri�t� dans configuration (COMPRESSED,BASE,EXTERN_FILE,IMPORTANT)
int cdHeader_IsCompressed(coreDumpHeader * cdptr);
int cdHeader_isBaseBlock(coreDumpHeader * cdptr);
int cdHeader_isExternFile(coreDumpHeader * cdptr);
int cdHeader_isImportant(CoreDumpHeader * cdptr);

//active les flag de propr�t� dans configuration(IMPORTANT, COMPRESSED,EXTERN_FILE)
void cdHeader_setImportant(CoreDumpHeader * cdptr);
void cdHeader_SetCompressed(coreDumpHeader * cdptr);
void cdHeader_SetExternFile(coreDumpHeader * cdptr);

//compte le nombre de frame dans les blocks
int64_t  cdHeader_FrameInBlock(coreDumpHeader * cdptr);

//Place le charriot de fst � la position StartPosition
void cdHeader_goStartIndex_F(coreDumpHeader * cdptr, FILE * fst);
//Place le charriot de fst � la position de fin (StartPosition+Totalsize)
void cdHeader_goBlockEnd_F(coreDumpHeader * cdptr, FILE * fst);

//fini le block et met � jour le header dans le fichier fst.
void cdHeader_TerminateBlock(coreDumpHeader * cdptr, FILE * fst);

//met � jour le header(place automatiquement � la bonne place)
void cdHeader_UpdateHeader(coreDumpHeader * cdptr, FILE * fst);//s
void cdHeader_UpdateHeader_no_restore(coreDumpHeader * cdptr, FILE * fst);//ne remet le fichier � la position qu'il avait avant la mise � jour

//met � jour les donn�s de la structure depuis les donn�s dans fst (en se basant sur StartPosition)
void cdHeader_UpdateFromFile(coreDumpHeader * cdptr, FILE * fst);

//Lis et �cris le header depuis fst =>
//le header est �cris et lu depuis la position du chariot de fst
void cdHeader_ReadHeader_F(coreDumpHeader * cdptr, FILE * fst);
void cdHeader_WriteHeader_F(coreDumpHeader * cdptr, FILE * fst);

//donne la position pour la frame f dans le fichier fst
int64_t  cdHeader_PredictFramePosition_F(coreDumpHeader * cdptr, int64_t  f, FILE * fst);
//v�rifie le predicteur pour le dernier ajout
int cdHeader_PredictHit_F(coreDumpHeader * cdptr, FILE * fst);

//recherche la fin du block ou le chariot de fst est plac�
int64_t  cdHeader_SearchBlockEnd_F(coreDumpHeader * cdptr, FILE * fst);

//ajoute les donn�s dans le header
//cdptr : le  header
//Pos :  position du block ajout�
//size : taille du block ajout�
//frameinblock_a : nombre de frame dans le block
//firstFrameofBlock : premier
void cdHeader_addBlockSize(coreDumpHeader * cdptr, int64_t  Pos, int64_t  size, int64_t  frameinblock_a, int64_t  firstFrameOfBlock);

//retourne la position de fin des block
int64_t  cdHeader_BlockEnd(coreDumpHeader * cdptr);

//marque la fin d'un block � l'aide de MARK_CHAR
void cdHeader_BlockMarker_F(FILE * fst);

//affiche les information d'un Block
void cdHeader_printInfo(coreDumpHeader * cdptr);
