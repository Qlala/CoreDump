#pragma once
#include<stdio.h>
#include <stdio.h>
#include "CoreDumpType.h"
#include "CoreDumpBlock.h"


//type pour les pointeur de fonction
typedef int(*EncodingNeeded_fun_T)(int depth);
typedef int(*MaxBlockCountReach_fun_T)(int depth, int count);
typedef int(*SeparatedFileNeeded_fun_T)(int depth);
typedef char* (*SeperateFileName_fun_T)(int depth, int nb);

//type pour les pointeur de fonction pour la compression
typedef int(*Encode_FF_fun_T)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
typedef int(*Encode_PF_fun_T)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);


//cr�er un nouveau fichier avec le CoreDumpTop cdtpr et le nom filename
CoreDumpFile * cdTop_CreateNewDumpFile(CoreDumpTop * cdtptr, char * filename);

//gestion du multi threading
//cdtptr: le coreDumpTop avec lequel on int�ragis
//incr�mente le s�maphore
void cdTop_IncSema(CoreDumpTop* cdtptr);
//d�cr�mente le s�maphore
void cdTop_ReleaseSema(CoreDumpTop* cdtptr);
//essaye d'attendre le s�maphore (attend le fait qu'il soit � 0).
//renvoie 1 si le s�maphore est � 0
int cdTop_TryWaitSema(CoreDumpTop * cdtptr);

//ferme le fichier
void cdTop_CloseDumpFile(CoreDumpFile * cdfptr);

//verifie si le niveau depth demande un encodage
int cdTop_EncodingNeeded(CoreDumpTop * cdtptr, int depth);
//defini si le nombre de bloc count pour le niveau depth demande un encodage
int cdTop_MaxBlockCountReach(CoreDumpTop * cdtptr, int depth, int count);
//regarde si un  fichier s�par� est n�c�ssaire pour le niveau depth
int cdTop_SeparateFileNeeded(CoreDumpTop * cdtptr, int depth);
//g�nere le non de fichier pour le niveau depth et le bloc num�ro nb de ce niveau
char* cdTop_SeparateFileName(CoreDumpTop * cdtptr, int depth, int nb);
//fini l'arbre du fichier cdfptr
void cdTop_FinishTree(CoreDumpFile * cdfptr);

//ajout d'une donn� sous forme de fichier ou de pointeur
//void cdTop_addFrame_F(CoreDumpFile * cdfptr, FILE * frame);(demande des modification pour �tre rendu active)
void cdTop_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

//n�ttoie l'objet top pour lui lib�r� la m�moire alouer par l'initialisation cdTop_BlankImplementation
void cdTop_CleanBlankTop(CoreDumpTop * cdtptr);

//Initialise un CoreDumpTop
CoreDumpTop * cdTop_BlankImplementation(int max_block_count);
