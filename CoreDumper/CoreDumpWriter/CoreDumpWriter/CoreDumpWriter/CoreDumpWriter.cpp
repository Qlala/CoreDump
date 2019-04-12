// CoreDumpWriter.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//


#include <stdio.h>
#include <iostream>
#include <time.h>
extern "C" {
#include "CoreDumpImpl.h"
#include "CoreDumpUtils.h"
}
int main_test_frame_asF()
{
	CoreDumpFile* test = cd_CreateFile((char*)"test.set");

	FILE* test_frame = fopen("test_frame_ref_forced","rb");
	_fseeki64(test_frame, 0, SEEK_END);
	size_t cycle_size = _ftelli64(test_frame);
	std::cout << "taille du cycle :"<<cycle_size<<" taille attendu :"<< 300*cycle_size<<std::endl;
	_fseeki64(test_frame, 0, SEEK_SET);
	for (int i = 0; i < 300; i++) {
		cd_addFrame_F(test, test_frame);
		printf("frame %i\n", i);
	}
	cd_CloseFile(test);
	return 0;
}




int main()
{
	char string_buff[100];
	/*
	CoreDumpTop* top = cdTop_BlankImplementation();

	CoreDumpFile* test = cdTop_CreateNewDumpFile(top, (char*)"test.set");
	
	cdSepFile_SetTop(test, 1);
	cdDef_Enc_SetTop_func(top);
	*/
	CoreDumpFile* test = cd_CreateFile((char*)"test.set");

	FILE* test_frame = fopen("test_frame_ref_forced", "rb");
	fseek(test_frame, 0, SEEK_END);
	int64_t frame_size=ftell(test_frame);
	fseek(test_frame, 0, SEEK_SET);
	char* frame_buff = new char[frame_size];
	fread(frame_buff, 1, frame_size, test_frame);
	size_t cycle_size = frame_size;
	std::cout << "taille du cycle :" << cycle_size << " taille attendu :" << 300 * cycle_size << std::endl;
	_fseeki64(test_frame, 0, SEEK_SET);
	struct timespec t1,t2;
	for (int i = 0; i < 300; i++) {
		
		timespec_get(&t1, TIME_UTC);

		cd_addFrame_P(test, frame_buff,frame_size);
		timespec_get(&t2, TIME_UTC);
		std::cout << "frame " << i << " en " << get_time_diff(string_buff, t1, t2) << std::endl;
		
	}
	cd_CloseFile(test);
	return 0;
}
// Exécuter le programme : Ctrl+F5 ou menu Déboguer > Exécuter sans débogage
// Déboguer le programme : F5 ou menu Déboguer > Démarrer le débogage

// Conseils pour bien démarrer : 
//   1. Utilisez la fenêtre Explorateur de solutions pour ajouter des fichiers et les gérer.
//   2. Utilisez la fenêtre Team Explorer pour vous connecter au contrôle de code source.
//   3. Utilisez la fenêtre Sortie pour voir la sortie de la génération et d'autres messages.
//   4. Utilisez la fenêtre Liste d'erreurs pour voir les erreurs.
//   5. Accédez à Projet > Ajouter un nouvel élément pour créer des fichiers de code, ou à Projet > Ajouter un élément existant pour ajouter des fichiers de code existants au projet.
//   6. Pour rouvrir ce projet plus tard, accédez à Fichier > Ouvrir > Projet et sélectionnez le fichier .sln.
