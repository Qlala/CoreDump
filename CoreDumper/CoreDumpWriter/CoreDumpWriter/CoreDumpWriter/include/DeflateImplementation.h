#pragma once
//module de compression deflate
#include "CoreDumpType.h"
//Retire ce qui � �t�  alou� par l'initialisation du module
void cdDef_CleanTop(CoreDumpTop * cdtptr);
//initalise le module deflate
//encode_depth_level : unique niveau auquel les blocks seront compress�s
void cdDef_Enc_SetTop_func(CoreDumpTop * cdtptr, int encode_depth_level);

