#pragma once
#include "CoreDumpType.h"
//module de gestion des Deltas
//il intéragit avec le CoreDumpTop pour se rajouter parmis les modules actifs.

//active les deltas
//aloue des information dans le CoreDumpTop
void cdDelta_SetImpl(CoreDumpTop * cdtptr);

//désaloue les données alouées par l'activation
void cdDelta_CleanTop(CoreDumpTop * cdtptr);
