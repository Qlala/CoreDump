
#include <string.h>
#include <Windows.h>
#include "SeparatedFileImplementation.h"
#include <math.h>

struct sepfile_struct_S{
	char** file_trans;
	int fragment_total;
};
typedef struct sepfile_struct_S sepfile_struct;
int cdSepFile_createDirectory(char* dir) {
#ifdef _WIN32
	return ERROR_ALREADY_EXISTS == CreateDirectoryA(dir, NULL);
#endif // _WIN32	
}


char* cdSepFile_gen(int depth,int nb,void* x) {
	char* file_name = *(((sepfile_struct*)x)->file_trans);
	int* count = &(((sepfile_struct*)x)->fragment_total);
	(*count)++;
	char* file_name_without_ext = malloc(strlen(file_name)+2);
	char* file_head_end=strrchr(file_name, '.');
	size_t head_size = (size_t)file_head_end - (size_t)file_name;
	memcpy(file_name_without_ext, file_name, head_size);
	file_name_without_ext[head_size] = '\0';

	//make directory if it doesn't exist
	cdSepFile_createDirectory(file_name_without_ext);

	char* sep_file_name_form = malloc(2*strlen(file_name) + 30);
	size_t len = 2 * strlen(file_name) + 30 + ceil(log10((*count)));
	char* sep_file_name = malloc(len);
	sep_file_name_form[0] = '\0';
	strcat(sep_file_name_form, file_name_without_ext);
	strcat(sep_file_name_form, "/");
	strcat(sep_file_name_form, file_name_without_ext);
	strcat(sep_file_name_form, ".part%i.cd_frag");
	free(file_name_without_ext);
	snprintf(sep_file_name, len ,sep_file_name_form,*count);
	free(sep_file_name_form);
	return sep_file_name;
}


int cdSepFile_needed(int depth,void* x) {return depth ==*(int*)x ;};

void cdSepFile_SetTop(CoreDumpFile* cdfptr,int file_sep_level) {
	cdfptr->top->SeparateFileName = cdSepFile_gen;
	cdfptr->top->SepFileParam_SeparateFileName = malloc(sizeof(sepfile_struct));
	((sepfile_struct*)(cdfptr->top->SepFileParam_SeparateFileName))->file_trans = &cdfptr->fileName;
	((sepfile_struct*)(cdfptr->top->SepFileParam_SeparateFileName))->fragment_total = 0;
	cdfptr->top->SeparateFileNeeded = cdSepFile_needed;
	cdfptr->top->SepFileParam_SeparateFileNeeded = malloc(sizeof(int));
	*(int*)(cdfptr->top->SepFileParam_SeparateFileNeeded) = file_sep_level;
	//cdSepFile_gen(0, 2, cdfptr->top->SepFileParam_SeparateFileName);
	//cdTop_SeparateFileName(cdfptr->top, 0, 2);
}

//free allocation made by SetTop
void cdSepFile_CleanTop(CoreDumpTop * cdtptr) {
	if(cdtptr->SepFileParam_SeparateFileName!=NULL)
		free(cdtptr->SepFileParam_SeparateFileName);
	if (cdtptr->SepFileParam_SeparateFileNeeded != NULL)
		free(cdtptr->SepFileParam_SeparateFileNeeded);

}