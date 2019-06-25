#pragma once
#include "CoreDumpHeader.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif // _WIN32


//type qui gère les noeud de l'abre de recherche
typedef struct CoreDumpBlock_S CoreDumpBlock;
struct CoreDumpBlock_S
{
	coreDumpHeader* header_ptr;//gestion du header
	struct CoreDumpBlock_S* child;//enfant actuelle
	FILE* nodeFile;//fichier contenant le block
	char* nodeFileName;//nom de se fichier
	int depth;//profondeur(0 est le niveau des feuille et >0 sont les branches)
	int blockCount;//nombre de block
};


//type de gestion des modules pour l'abre de bllock
//en modifiant les fonction pointeur on peut implémenter les contraintes et les compression de sont choix
typedef struct CoreDumpTop_S CoreDumpTop;
struct CoreDumpTop_S
{
	//fonction et paramètre dédié pour la gestion de la nécassité de compressé
	//la fonction renvoie TRUE si il faut compressé pour le niveau depth sinon FALSE
	void* EncodingNeeeded_param;
	int(*EncodingNeeded)(int depth,void* param);
	
	//fonction et paramètre dédié à la gestion du nombre de block(enfant) maximum par niveau
	//renvoie TRUE si le nombre de block est atteint sinon FALSE (depth: niveau, count=nombre de block)
	void* MaxBlockCountReach_param;
	int(*MaxBlockCountReach)(int depth, int count,void* param);
	
	//gestion des fichier différent
	//paramètre et fonction
	//renvoie TRUE si pour le niveau depth il y a besoin d'un fichier séparée
	void* SepFileParam_SeparateFileNeeded;
	int(*SeparateFileNeeded)(int depth,void* param);
	//gènere le nom du fichier séparé pour le niveau depth le nombre de block nb
	void* SepFileParam_SeparateFileName;
	char* (*SeparateFileName)(int depth, int nb,void*param);
	//fonction de compression
	//compresse input de taille in_l en output de taile out_l
	int(*Encode_FF)(FILE* input, FILE* output, int64_t  in_l, int64_t * out_l);
	int(*Encode_PF)(char* input, FILE* output, int64_t  in_l, int64_t * out_l);
	
	//fonction appeleé à chaque ajout de donné
	//return 1 if copy is not needed
	//dst : fichier où ajouté des données
	//cdhptr: header du block feuille (BASE)
	//frame : donnée ajouté au fichier (cycle)
	//size : taille des donnés effectivement ajoutées
	void* par_frame_operationParam;
	int(*per_frame_operation_P)(FILE* dst,CoreDumpHeader* cdhptr, CoreDumpTop* cdtptr, char* frame, int64_t* size);

	//Semaphore
#ifdef _WIN32
	HANDLE protection_Mutex;
	volatile int64_t semaphore;
#else
	pthread_mutex_t p_mutex;
	volatile int64_t semaphore;

#endif // _WIN32

};

//strcuture de gestion des fichier
typedef struct CoreDumpFile_S CoreDumpFile;
struct CoreDumpFile_S
{
	FILE* file;//fichier principale de l'arbre
	char* fileName;//nom du fichier
	CoreDumpTop* top;//structure de gestion des modules
	CoreDumpBlock* tree;//racine de l'arbre

};
