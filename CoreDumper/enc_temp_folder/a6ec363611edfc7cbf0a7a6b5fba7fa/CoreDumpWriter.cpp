// CoreDumpWriter.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include <random>
#include <filesystem>
extern "C" {
#include "CoreDumpImpl.h"
#include "CoreDumpUtils.h"
#include "CoreDumpTop.h"
}
#define TEST_FRAME_COUNT 1000
void generate_test_frame(const char* filename,size_t frame_size ,double proba_sortie_de_serie) {
	FILE* myfile = fopen(filename, "wb");
	//std::random_device rd;
	std::minstd_rand0 gen(clock());
	std::uniform_real_distribution<float> dis_proba(0., 1.);
	std::uniform_int_distribution<unsigned short> dis_int(0X00, 0XFF);
	char ser = dis_int(gen);
	for (size_t i = 0; i < frame_size; i++) {
		if (dis_proba(gen) < proba_sortie_de_serie) {
			ser = dis_int(gen);
		}
		fwrite(&ser, 1, 1, myfile);
	}
	fclose(myfile);
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
	//std::random_device rd;
	std::minstd_rand0 gen(clock());
	std::uniform_real_distribution<float> dis_proba(0., 1.);
	std::uniform_int_distribution<unsigned short> dis_int(0X00, 0XFF);
	for (int64_t i = 0; i < size/8; i+=1) {
		if (dis_proba(gen) < proba_change) {
			char a= dis_int(gen);
			printf("alteration:%lli %hhu=>%hhu\n", i,frame[i],a);
			frame[i] = a;
			
			i += 10000;
		}
	}
	//frame[0] = dis_char(gen);
}
size_t computeSizeOfDir(const char* path) {
	
	namespace fs = std::experimental::filesystem;
	size_t size = 0;
	for (fs::recursive_directory_iterator it(path);
		it != fs::recursive_directory_iterator();
		++it)
	{
		try {
			if (!fs::is_directory(*it))
				size += fs::file_size(*it);
		}
		catch (...) {
			printf("error in compute size of Dir\n");
		}

	}
	return size;
}


void push_time_csv(const char* file_name, int64_t n,int64_t t_ns, int64_t t_s,int64_t cycle_size, size_t size)
{
	try {
		
		
		
		using namespace std;
		fstream file;
		file.open(file_name, ios::app | ios::out);
		if (file.is_open()) {
			file << n << ';' << (t_ns < 0 ? t_ns + 1000000000 : t_ns) << ";" << t_s << ';' << size << ";" << (size*100.f / ((n + 1)* cycle_size)) << ";" << endl;
			file.close();
		}
	}
	catch (...) {
		printf("error in pushing data to csv\n");
	}
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
	//freopen("log.txt", "w+", stdout);
	remove("../result/result.csv");
	std::experimental::filesystem::remove_all("test/");
	FILE* test_frame = fopen("test_frame_ref_forced", "rb");
	if (!test_frame) {
		generate_test_frame("test_frame_ref_forced", 128*1024*1024,0.05);
		test_frame = fopen("test_frame_ref_forced", "rb");
	}
	CoreDumpFile* test = cd_CreateFile((char*)"test.set");


	fseek(test_frame, 0, SEEK_END);
	int64_t frame_size=ftell(test_frame);
	fseek(test_frame, 0, SEEK_SET);
	char* frame_buff = new char[frame_size];
	fread(frame_buff, 1, frame_size, test_frame);
	size_t cycle_size = frame_size;
	std::cout << "taille du cycle :" << cycle_size << " taille attendu :" << TEST_FRAME_COUNT * cycle_size << std::endl;
	_fseeki64(test_frame, 0, SEEK_SET);
	struct timespec t1,t2;
	int64_t sum_ns = 0;
	int64_t sum_s = 0;
	int64_t curr_size = 0;
	int i;
	for (i = 0; i < TEST_FRAME_COUNT; i++) {
		
		timespec_get(&t1, TIME_UTC);

		cd_addFrame_P(test, frame_buff,frame_size);
		timespec_get(&t2, TIME_UTC);
		std::cout << "frame " << i << " en " << get_time_diff(string_buff, t1, t2) << std::endl;
		alter_frame(frame_buff, frame_size, 0.000001);

		if (cdTop_TryWaitSema(test->top)) {
			curr_size = computeSizeOfDir("test/");
			curr_size += std::experimental::filesystem::file_size("test.set");
		}
		push_time_csv("../result/result.csv", i, t2.tv_nsec - t1.tv_nsec, t2.tv_sec - t1.tv_sec,frame_size,curr_size);
		sum_ns += (t2.tv_nsec - t1.tv_nsec);
		sum_s += (t2.tv_sec - t1.tv_sec);
	}
	double med_ns = sum_ns /i;
	double med_s = sum_s / i;
	printf("moyenne des résultat");
	printf("s=%f , ns=%f\n", med_s, med_ns);
	cd_CloseFile(test);
	return 0;
}

