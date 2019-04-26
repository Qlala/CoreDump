// CoreDumpWriter.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//


#include <stdio.h>
#include <iostream>
#include <time.h>
#include <ctime>
#include <random>
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

void alter_frame(char* frame, int64_t size,float proba_change) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis_proba(0., 1.);
	std::uniform_int_distribution<uint64_t> dis_int(0X00, -1);
	for (int64_t i = 0; i < size/8; i+=1) {
		if (dis_proba(gen) < proba_change) {
			((int64_t*)frame)[i] = dis_int(gen);
			printf("alteration:ll%i\n",i);
			i += 10000;
		}
	}
	//frame[0] = dis_char(gen);
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
	int64_t sum_ns = 0;
	int64_t sum_s = 0;
	for (int i = 0; i < 300; i++) {
		
		timespec_get(&t1, TIME_UTC);

		cd_addFrame_P(test, frame_buff,frame_size);
		timespec_get(&t2, TIME_UTC);
		std::cout << "frame " << i << " en " << get_time_diff(string_buff, t1, t2) << std::endl;
		alter_frame(frame_buff, frame_size, 0.000001);
		sum_ns += t2.tv_nsec - t1.tv_nsec;
		sum_s += t2.tv_sec - t1.tv_sec;
	}
	double med_ns = sum_ns / 300;
	double med_s = sum_s / 300;
	printf("s=lf% , ns=%lf", med_s, med_ns);
	cd_CloseFile(test);
	return 0;
}

