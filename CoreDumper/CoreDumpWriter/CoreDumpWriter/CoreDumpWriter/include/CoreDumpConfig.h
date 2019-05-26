#pragma once
#include "zlib-1.2.11/zlib.h"
#include <stdint.h>
//Configuration static du program => si library 
//CoreDumpUtils
#define VERBOSE 0//Verbosité du programme (presence d'une sortie console)
//CoreDumpHeader
#define MARK_CHAR_REF 0xAA//octet utilisé pour séparé les bloc
//CoreDumpBlock
#define USE_THREADED_ENCODE //active la paralelisation pour la compression (tout les cas possible d'echec sont géré)
#define BLOCK_BUFF_SIZE 1024//taille du buffer servant à la copie des block (peu utiliser).
//DeflateImplementation
#define CHUNK (1<<20)//taille du  buffer pour la déccompression
#define COMPRESSION_LEVEL Z_BEST_SPEED //Paramètre de compression (défini d'après ZLIB) default:Z_DEFAULT_COMPRESSION
//deltaImplementation
#define DELTA_THRESHOLD 0.05//seuil en fraction de la taille d'un cycle default:0.01 (plus que 0.5 semble être mauvais)
#define DELTA_WINDOW 256 //taille des block (fenêtre) pour le calcul des deltas 1024