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


//créer un nouveau fichier avec le CoreDumpTop cdtpr et le nom filename
CoreDumpFile * cdTop_CreateNewDumpFile(CoreDumpTop * cdtptr, char * filename);

//gestion du multi threading
//cdtptr: le coreDumpTop avec lequel on intéragis
//incrémente le sémaphore
void cdTop_IncSema(CoreDumpTop* cdtptr);
//décrémente le sémaphore
void cdTop_ReleaseSema(CoreDumpTop* cdtptr);
//essaye d'attendre le sémaphore (attend le fait qu'il soit à 0).
//renvoie 1 si le sémaphore est à 0
int cdTop_TryWaitSema(CoreDumpTop * cdtptr);

//ferme le fichier
void cdTop_CloseDumpFile(CoreDumpFile * cdfptr);

//verifie si le niveau depth demande un encodage
int cdTop_EncodingNeeded(CoreDumpTop * cdtptr, int depth);
//defini si le nombre de bloc count pour le niveau depth demande un encodage
int cdTop_MaxBlockCountReach(CoreDumpTop * cdtptr, int depth, int count);
//regarde si un  fichier séparé est nécéssaire pour le niveau depth
int cdTop_SeparateFileNeeded(CoreDumpTop * cdtptr, int depth);
//gènere le non de fichier pour le niveau depth et le bloc numéro nb de ce niveau
char* cdTop_SeparateFileName(CoreDumpTop * cdtptr, int depth, int nb);
//fini l'arbre du fichier cdfptr
void cdTop_FinishTree(CoreDumpFile * cdfptr);

//ajout d'une donné sous forme de fichier ou de pointeur
//void cdTop_addFrame_F(CoreDumpFile * cdfptr, FILE * frame);(demande des modification pour être rendu active)
void cdTop_addFrame_P(CoreDumpFile * cdfptr, char * frame, int64_t size_frame);

//néttoie l'objet top pour lui libéré la mémoire alouer par l'initialisation cdTop_BlankImplementation
void cdTop_CleanBlankTop(CoreDumpTop * cdtptr);

//Initialise un CoreDumpTop
CoreDumpTop * cdTop_BlankImplementation(int max_block_count);
