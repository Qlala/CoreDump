#pragma once
#include "CoreDumpHeader.h"
#include "CoreDumpType.h"
#include <stdlib.h>
#include <stdio.h>



//création d'un noeud de l'abre et initialisation de son header:
//fst: fichier ou l'abre est crée (la position du pointeur donne le lieu ou démarre le noeud dans le fichier)
//first_frame : numéro du premier élement du noeud (premier cycle du noeud)
//depth_a : profondeur du noeud (0=>BASE , >0 => branches)
//no_write : interdit l'écriture dans le fichier fst(le header est initialisé mais pas écris)
CoreDumpBlock * cdBlock_CreateNewChild_F(FILE * fst, int64_t  first_frame, int depth_a, int no_write);

//désaloue les pointeur de sur des CoreDumpBlock et les mets à NULL
void cdBlock_DeleteBlock(CoreDumpBlock ** cdbptr);

//non fonctionnelle (retiré à un commut précédent)
//int cdBlock_addFrame_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, FILE * frame);

//ajoute dans les donnés dans un block de l'abre (appel récursif)
//cdbptr : le block auquel on ajoute les donnés
//cdtptr : la structure de gestion des modules
//fst : le fichier ou sont ajouté des données
//frame : les données (cycle)
//frame_size : la taile des données
int cdBlock_addFrame_P(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, char * frame, int64_t frame_size);

//termine l'arbre => valide tout les header et les mets dans un état fini.
//cdbptr : block que l'on fini
//cdtptr : la structure de gestion des modules
//fst : fichier correspondant
void cdBlock_FinishTree_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst);



//fonction qui écrie le nom du fichier séparée du block cdptr dans le fichier fst
void cdBlock_WriteFileName_F(CoreDumpBlock * cdbptr, FILE * fst);

//fonction complexe qui termine un enfant et valide ses données dans le parent
void cdBlock_addChildBlock_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE * fst, int depth, int * blockCount, int force_copy);
void cdBlock_addChildBlockFile_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE ** node_fst, char * nodeFileName, int depth, int * blockCount, int force_copy);
void cdBlock_addChildBlockCopy_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE * src_fst, FILE * dst_fst, int depth, int * blockCount);
