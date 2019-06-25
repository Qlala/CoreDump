#include "CoreDumpTop.h"
#include "CoreDumpType.h"
//gestion du module de fichier externe

//ajout du module dans le CoreDumpTop
//le module int�ragit avec le CoreDumpFile pour r�cup�r� le nom du fichier principale
// cdfptr : Le CoreDumpFile auquel a �t� associ� le CoreDumpTop auquel on ajout le module
//file_sep_level : unique niveau de profondeur o� chaque block sera dans un fichier diff�rent
void cdSepFile_SetTop(CoreDumpFile * cdfptr, int file_sep_level);

//n�ttoie les allocations faite par l'ajout du module
void cdSepFile_CleanTop(CoreDumpTop * cdtptr);