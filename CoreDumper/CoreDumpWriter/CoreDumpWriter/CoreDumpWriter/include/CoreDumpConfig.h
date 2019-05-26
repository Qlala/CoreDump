#pragma once
#include "zlib-1.2.11/zlib.h"
#include <stdint.h>
//Configuration static du program => si library 
//CoreDumpUtils
#define VERBOSE 0//Verbosit� du programme (presence d'une sortie console)
//CoreDumpHeader
#define MARK_CHAR_REF 0xAA//octet utilis� pour s�par� les bloc
//CoreDumpBlock
#define USE_THREADED_ENCODE //active la paralelisation pour la compression (tout les cas possible d'echec sont g�r�)
#define BLOCK_BUFF_SIZE 1024//taille du buffer servant � la copie des block (peu utiliser).
//DeflateImplementation
#define CHUNK (1<<20)//taille du  buffer pour la d�ccompression
#define COMPRESSION_LEVEL Z_BEST_SPEED //Param�tre de compression (d�fini d'apr�s ZLIB) default:Z_DEFAULT_COMPRESSION
//deltaImplementation
#define DELTA_THRESHOLD 0.05//seuil en fraction de la taille d'un cycle default:0.01 (plus que 0.5 semble �tre mauvais)
#define DELTA_WINDOW 256 //taille des block (fen�tre) pour le calcul des deltas 1024