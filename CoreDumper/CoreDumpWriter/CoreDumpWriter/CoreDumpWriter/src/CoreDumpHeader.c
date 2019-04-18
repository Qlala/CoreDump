#include <stdio.H>
#include <stdlib.h>
#include "CoreDumpConfig.h"
#include "CoreDumpHeader.h"

const char MARK_CHAR = MARK_CHAR_REF;

coreDumpHeader* cdHeader_Create(int64_t start_index) {
	coreDumpHeader* cdptr = (coreDumpHeader*)malloc(sizeof(coreDumpHeader));
	cdptr->headerSize = CD_HEADER_SIZE;
	cdptr->totalSize=cdptr->headerSize;
	cdptr->predBlockSize = 0;
	cdptr->predFramePerBlock = 0;
	cdptr->firstFrame = 0;
	cdptr->lastFrame = 0;
	cdptr->lastAddedBlockPos=start_index;
	cdptr->firstFrameOfLastBlock=0;
	cdptr->configuration = 0;
	cdptr->startPosition=start_index;
	cdptr->lastAddedBlockSize=0;
	return cdptr;
}
void cdHeader_Delete(coreDumpHeader* cdptr) 
{
	free(cdptr);
}

int cdHeader_IsCompressed(coreDumpHeader* cdptr)
{
	return (cdptr->configuration & COMPRESSED) > 0;
}

int cdHeader_isBaseBlock(coreDumpHeader* cdptr)
{
	return (cdptr->configuration & BASE) > 0;
}
int cdHeader_isExternFile(coreDumpHeader* cdptr)
{
	return ((cdptr->configuration) & EXTERN_FILE) > 0;
}

void cdHeader_SetCompressed(coreDumpHeader* cdptr)
{
	cdptr->configuration |= COMPRESSED;
}
void cdHeader_SetExternFile(coreDumpHeader* cdptr)
{
	cdptr->configuration |= EXTERN_FILE;
}
int64_t  cdHeader_FrameInBlock(coreDumpHeader* cdptr) {
	return cdptr->lastFrame - cdptr->firstFrame;
}

void cdHeader_goStartIndex_F(coreDumpHeader* cdptr, FILE* fst)
{
	_fseeki64(fst, cdptr->startPosition, SEEK_SET);
}

void cdHeader_TerminateBlock(coreDumpHeader* cdptr, FILE* fst)
{
	cdptr->configuration |= FINISHED;
	cdHeader_UpdateHeader(cdptr, fst);
}

void cdHeader_UpdateHeader(coreDumpHeader* cdptr, FILE* fst)//ne chnage pas la position dans le stream
{
	int64_t  c_pos = _ftelli64(fst);
	cdHeader_goStartIndex_F(cdptr, fst);
	cdHeader_WriteHeader_F(cdptr, fst);
	_fseeki64(fst, c_pos, SEEK_SET);
}
void cdHeader_UpdateFromFile(coreDumpHeader* cdptr, FILE* fst) {
	int64_t  c_pos = _ftelli64(fst);
	cdHeader_goStartIndex_F(cdptr, fst);
	cdHeader_ReadHeader_F(cdptr, fst);
	_fseeki64(fst, c_pos, SEEK_SET);
}



void cdHeader_ReadHeader_F(coreDumpHeader* cdptr, FILE* fst)//le stream est mis à la fin du header ( donc la fonction altère l'état du stream)
{
	cdHeader_goStartIndex_F(cdptr, fst);
	//byte[] bytes = new byte[8];
	//input.Read(bytes, 0, 8);
	//totalSize = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->totalSize), sizeof(cdptr->totalSize), 1, fst);
	//input.Read(bytes, 0, 8);
	//predBlockSize = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->predBlockSize), sizeof(cdptr->predBlockSize), 1, fst);
	//input.Read(bytes, 0, 8);
	//predFramePerBlock = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->predFramePerBlock), sizeof(cdptr->predFramePerBlock), 1, fst);
	//input.Read(bytes, 0, 8);
	//firstFrame = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->firstFrame), sizeof(cdptr->firstFrame), 1, fst);
	//input.Read(bytes, 0, 8);
	//lastFrame = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->lastFrame), sizeof(cdptr->lastFrame), 1, fst);
	//input.Read(bytes, 0, 8);
	//lastAddedBlockPos = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->lastAddedBlockPos), sizeof(cdptr->lastAddedBlockPos), 1, fst);
	//input.Read(bytes, 0, 8);
	//firstFrameOfLastBlock = BitConverter.ToInt64(bytes, 0);
	fread(&(cdptr->firstFrameOfLastBlock), sizeof(cdptr->firstFrameOfLastBlock), 1, fst);
	//input.Read(bytes, 0, 1);
	//configuration = bytes[0];
	fread(&(cdptr->configuration), sizeof(cdptr->configuration), 1, fst);
}


void cdHeader_WriteHeader_F(coreDumpHeader* cdptr, FILE* fst)//écrie le header dans un stream : n'est pas responsable de la position dans le stream ou du fait que le stream soit inscriptible
{
	fwrite(&(cdptr->totalSize), sizeof(cdptr->totalSize), 1, fst);
	//byte[] bytes = BitConverter.GetBytes(totalSize);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->predBlockSize), sizeof(cdptr->predBlockSize), 1, fst);
	//bytes = BitConverter.GetBytes(predBlockSize);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->predFramePerBlock), sizeof(cdptr->predFramePerBlock), 1, fst);
	//bytes = BitConverter.GetBytes(predFramePerBlock);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->firstFrame), sizeof(cdptr->firstFrame), 1, fst);
	//bytes = BitConverter.GetBytes(firstFrame);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->lastFrame), sizeof(cdptr->lastFrame), 1, fst);
	//bytes = BitConverter.GetBytes(lastFrame);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->lastAddedBlockPos), sizeof(cdptr->lastAddedBlockPos), 1, fst);
	//bytes = BitConverter.GetBytes(lastAddedBlockPos);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->firstFrameOfLastBlock), sizeof(cdptr->firstFrameOfLastBlock), 1, fst);
	//bytes = BitConverter.GetBytes(firstFrameOfLastBlock);
	//output.Write(bytes, 0, bytes.Length);

	fwrite(&(cdptr->configuration), sizeof(cdptr->configuration), 1, fst);
	//bytes = BitConverter.GetBytes(configuration);
	//output.Write(bytes, 0, 1);
}



//retourne la position du prochain block(si le header à le flags BASE alors il s'agit de la frame que l'on cherche)
int64_t  cdHeader_PredictFramePosition_F(coreDumpHeader* cdptr, int64_t f, FILE* fst)
{
	if ((cdptr->configuration & FINISHED) > 0)//si on a fini le block
	{
		if (cdptr->predFramePerBlock != 0 && ((f - cdptr->firstFrame) / cdptr->predFramePerBlock == 0))
		{
			return cdptr->startPosition + cdptr->headerSize;
		}
		else if (f > cdptr->firstFrame && f < cdptr->lastFrame)//last frame est bien la dernière frame du bloc
		{
			if (f >= cdptr->firstFrameOfLastBlock)
			{//cas particulier

				return cdptr->startPosition + cdptr->lastAddedBlockPos;
			}
			else//cas classique
			{
				int64_t  approx = cdptr->startPosition + cdptr->headerSize + ((cdptr->predBlockSize) * ((f - cdptr->firstFrame) / cdptr->predFramePerBlock)) - 1;
				int64_t  last_pos = _ftelli64(fst);
				char b;
				int i = 0;
				
				_fseeki64(fst, approx, SEEK_SET);
				//st.Seek(approx, SeekOrigin.Begin);
				do
				{
					b = fgetc(fst);
					i++;
				} while (b != MARK_CHAR && i < cdptr->predBlockSize && !feof(fst));//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
				//TODO: possiblement erooné ou sur-limité
				int64_t  pos = (feof(fst)) + +((feof(fst)) ? 0 : 0);
				_fseeki64(fst, last_pos, SEEK_SET);//on remet en bonne position
				return pos;
			}
		}
		else
		{
			return -1;//erreur on n'est pas dans le bon block
		}
	}
	else
	{
		if (cdptr->predFramePerBlock != 0 && (f - cdptr->firstFrame) / cdptr->predFramePerBlock == 0)
		{
			return cdptr->startPosition + cdptr->headerSize;
		}
		else if (f >= cdptr->firstFrame && f < cdptr->lastFrame)//comme d'habitude
		{

			int64_t  approx = cdptr->startPosition + cdptr->headerSize + (cdptr->predBlockSize * ((f-cdptr->firstFrame) / cdptr->predFramePerBlock))-1;
			int64_t  last_pos = _ftelli64(fst);
			char b;
			//byte[] b = new byte[1];
			int i = 0;
			clearerr(fst);
			_fseeki64(fst, approx, SEEK_SET);
			//st.Seek(approx, SeekOrigin.Begin);

			do
			{
				b=fgetc(fst);
				
				i++;
			} while (b != MARK_CHAR && i < cdptr->predBlockSize && !feof(fst));//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
			if (b != MARK_CHAR) {
				printf("marqueur pas trouve\n");
				if (b == EOF) {
					printf("fin de fichier possiblement atteinte\n");
				}
			}
			if (feof(fst)) {
				printf("fin du fcihier atteint\n");
			}
			
			//TODO: possiblement erooné ou sur-limité
			int64_t  pos = _ftelli64(fst);
			_fseeki64(fst, last_pos, SEEK_SET);//on remet en bonne position
			return pos;

		}
		else//on est dans le dernier block non comprimé
		{

			return cdptr->startPosition + cdptr->totalSize;
		}
	}
}

//retourne si la prédiction est fausse pour le prochain Block
//TODO a verifier => pas sur (dépend de l'ajout pour savoir si il sera invalid )
int cdHeader_PredictHit_F(coreDumpHeader* cdptr, FILE* fst)
{
	int64_t  approx_fin = cdptr->startPosition+cdptr->headerSize + cdptr->predBlockSize * ((cdptr->lastFrame - 1 - cdptr->firstFrame) / cdptr->predFramePerBlock);//approx de la prediction de la fin du block//
	int64_t  pred_fin = cdHeader_PredictFramePosition_F(cdptr, cdptr->lastFrame - 1, fst);
	int64_t  after_end = cdHeader_PredictFramePosition_F(cdptr, cdptr->lastFrame, fst);//predicition de fin
	int64_t  pred_debut = cdHeader_PredictFramePosition_F(cdptr, cdptr->firstFrameOfLastBlock, fst);//predicition du début
	if (!(pred_debut == (cdptr->lastAddedBlockPos + cdptr->startPosition) && ((pred_fin == (cdptr->lastAddedBlockPos + cdptr->startPosition) && after_end >= cdptr->totalSize + cdptr->startPosition) || cdptr->predFramePerBlock == 1)))
	{
		printf("echec predicteur : \n");
	}
	else {
		printf("reussite predicteur : \n");
	}
		int64_t  raw_approx = cdptr->startPosition + cdptr->headerSize + (cdptr->predBlockSize * ((cdptr->firstFrameOfLastBlock - cdptr->firstFrame) / cdptr->predFramePerBlock));
		//Console.WriteLine("echec de predicteur : r_a=" + raw_approx + " approx_fin=" + approx_fin + " pred=" + pred_debut + " after=" + after_end + "lastblock_pos=" + lastAddedBlockPos + ":real=" + (lastAddedBlockPos + startPosition) + " fFrameofLastBlock=" + firstFrameOfLastBlock + " fperblock=" + predFramePerBlock + "predbsize=" + predBlockSize + " totalsize=" + totalSize + " sp=" + StartPosition);
		printf("-information predicteur : r_a=%lli approx_fin=%lli pred=%lli pred_fin=%lli after=%lli lastblock_pos=%lli:real=%lli fFrameofLastBlock=%lli fperblock=%lli predbsize=%lli totalsize=%lli sp=%lli \n",raw_approx,approx_fin,pred_debut,pred_fin,after_end,cdptr->lastAddedBlockPos,cdptr->lastAddedBlockPos+cdptr->startPosition,cdptr->firstFrameOfLastBlock,cdptr->predFramePerBlock,cdptr->predBlockSize,cdptr->totalSize, cdptr->startPosition);
	//}

	return pred_debut == (cdptr->lastAddedBlockPos + cdptr->startPosition) && ((pred_fin == (cdptr->lastAddedBlockPos + cdptr->startPosition) && after_end >= cdptr->totalSize + cdptr->startPosition) || cdptr->predFramePerBlock == 1);//on verifie qu'on aurra le bon résultat.
}
int64_t  cdHeader_SearchBlockEnd_F(coreDumpHeader* cdptr, FILE* fst)
{
	int i = 0;
	int64_t  last_pos = _ftelli64(fst);
	char b;
	do
	{
		b = fgetc(fst);
		i++;
	} while (b != MARK_CHAR && i < cdptr->predBlockSize + 1 && !feof(fst) && _ftelli64(fst) < cdptr->startPosition + cdptr->totalSize);//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
	int64_t  end_pos = _ftelli64(fst) - 1;
	_fseeki64(fst, last_pos, SEEK_SET);
	return end_pos;
}

void cdHeader_addBlockSize(coreDumpHeader* cdptr, int64_t  Pos, int64_t  size, int64_t  frameinblock_a, int64_t  firstFrameOfBlock)
{//fonction qui met à jour les valeur pour un block avec une taille size et framePerBlock à l'interieur
	if (cdptr->predBlockSize == 0)//initialisation
	{
		cdptr->predBlockSize = size;
		cdptr->predFramePerBlock = frameinblock_a;
		if (firstFrameOfBlock >= 0) cdptr->firstFrame = firstFrameOfBlock;//utili pour les sous block qui ne peuvent pas avoir cette information
		cdptr->lastFrame = cdptr->firstFrame + frameinblock_a;
		cdptr->firstFrameOfLastBlock = cdptr->firstFrame;
	}
	else
	{

		if (firstFrameOfBlock >= 0)
		{
			cdptr->firstFrameOfLastBlock = firstFrameOfBlock;
		}
		else
		{
			cdptr->firstFrameOfLastBlock = cdptr->lastFrame;//on déduit la première frame de la dernière du bloc d'avant.
		}
		cdptr->lastFrame += frameinblock_a; ;
	}
	cdptr->totalSize += size;
	cdptr->lastAddedBlockPos = Pos - cdptr->startPosition;//pos se repère par rapport a startpostion
	cdptr->lastAddedBlockSize = size;
}
int64_t  cdHeader_BlockEnd(coreDumpHeader* cdptr)
{
	return cdptr->startPosition + cdptr->totalSize;
}
void cdHeader_BlockMarker_F(FILE* fst)
{
	fputc(MARK_CHAR, fst);
}