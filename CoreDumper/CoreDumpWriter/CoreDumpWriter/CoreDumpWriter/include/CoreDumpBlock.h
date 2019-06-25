#pragma once
#include "CoreDumpHeader.h"
#include "CoreDumpType.h"
#include <stdlib.h>
#include <stdio.h>



//cr�ation d'un noeud de l'abre et initialisation de son header:
//fst: fichier ou l'abre est cr�e (la position du pointeur donne le lieu ou d�marre le noeud dans le fichier)
//first_frame : num�ro du premier �lement du noeud (premier cycle du noeud)
//depth_a : profondeur du noeud (0=>BASE , >0 => branches)
//no_write : interdit l'�criture dans le fichier fst(le header est initialis� mais pas �cris)
CoreDumpBlock * cdBlock_CreateNewChild_F(FILE * fst, int64_t  first_frame, int depth_a, int no_write);

//d�saloue les pointeur de sur des CoreDumpBlock et les mets � NULL
void cdBlock_DeleteBlock(CoreDumpBlock ** cdbptr);

//non fonctionnelle (retir� � un commut pr�c�dent)
//int cdBlock_addFrame_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, FILE * frame);

//ajoute dans les donn�s dans un block de l'abre (appel r�cursif)
//cdbptr : le block auquel on ajoute les donn�s
//cdtptr : la structure de gestion des modules
//fst : le fichier ou sont ajout� des donn�es
//frame : les donn�es (cycle)
//frame_size : la taile des donn�es
int cdBlock_addFrame_P(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst, char * frame, int64_t frame_size);

//termine l'arbre => valide tout les header et les mets dans un �tat fini.
//cdbptr : block que l'on fini
//cdtptr : la structure de gestion des modules
//fst : fichier correspondant
void cdBlock_FinishTree_F(CoreDumpBlock * cdbptr, CoreDumpTop * cdtptr, FILE * fst);



//fonction qui �crie le nom du fichier s�par�e du block cdptr dans le fichier fst
void cdBlock_WriteFileName_F(CoreDumpBlock * cdbptr, FILE * fst);

//fonction complexe qui termine un enfant et valide ses donn�es dans le parent
void cdBlock_addChildBlock_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE * fst, int depth, int * blockCount, int force_copy);
void cdBlock_addChildBlockFile_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE ** node_fst, char * nodeFileName, int depth, int * blockCount, int force_copy);
void cdBlock_addChildBlockCopy_F(coreDumpHeader * src_cdptr, coreDumpHeader * dst_cdptr, CoreDumpTop * cdtptr, FILE * src_fst, FILE * dst_fst, int depth, int * blockCount);
