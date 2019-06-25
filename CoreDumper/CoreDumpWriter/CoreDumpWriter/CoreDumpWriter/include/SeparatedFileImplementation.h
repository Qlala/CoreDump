#include "CoreDumpTop.h"
#include "CoreDumpType.h"
//gestion du module de fichier externe

//ajout du module dans le CoreDumpTop
//le module intéragit avec le CoreDumpFile pour récupéré le nom du fichier principale
// cdfptr : Le CoreDumpFile auquel a été associé le CoreDumpTop auquel on ajout le module
//file_sep_level : unique niveau de profondeur où chaque block sera dans un fichier différent
void cdSepFile_SetTop(CoreDumpFile * cdfptr, int file_sep_level);

//néttoie les allocations faite par l'ajout du module
void cdSepFile_CleanTop(CoreDumpTop * cdtptr);