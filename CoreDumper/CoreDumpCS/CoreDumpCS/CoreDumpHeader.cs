using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using SevenZip;
using System.Runtime;
namespace CoreDumper
{
    abstract class CoreDumpHeader : IDisposable//ne décris que le comportement du header
    {
        Int64 headerSize = 8 * 7 + 1;
        Int64 totalSize;//prend en compte le header
        internal Int64 predBlockSize = 0;//absent si NO_PRED
        internal Int64 predFramePerBlock = 0;//absent si NO_PRED
        Int64 firstFrame = 0;//TODO => penser au cas avec peu de noeud dans l'abre : le prédicteur consome beaucoup de donner
        Int64 lastFrame = 0;//dernière frame ajouté au tableau de block si fini , sinon  c'est la dernière frame du dernier block qui a été ajouté
        Int64 lastAddedBlockPos;//par rapport à start Posotion
        Int64 firstFrameOfLastBlock;
        protected byte configuration = 0;
        public enum config_flags : byte { COMPRESSED = 0x1, FINISHED = 0x2, BASE = 0x4,NO_PRED=0x8,EXTERN_FILE=0x10 ,IMPORTANT=0x80};
        //EXTERN_FILE => the block only contains file string (BLOCK MARK maybe subituted with \0 )
        //
        //Header
        /*
        ||totalSize(8)|predBlockSize(8)|predFramePerBlock(8)|firstFrame(8)|lastFrame(8)|lastAddedBlockPos(8)|configuration(1);
         0      Hsize          TotalSize
         |      |              |
        \ /    \ /            \ /
        |Header| |BLOCKS .... | UNFINISHED_BLOCKS |

        
       */
        //not in the header => deduce by file start position

        //les prédiction et les POS se repère par rapport au début du fichier et non par rapport à start pos
        Int64 startPosition;//avant le header;
        Int64 lastAddedBlockSize;

        const byte MARK_CHAR = 0xAA;

        public long TotalSize { get => totalSize; }
        public long StartPosition { get => startPosition; }
        public long FrameInBlock { get => lastFrame - firstFrame; }
        internal long FirstFrame { get => firstFrame; set { firstFrame = value; if (value > lastFrame) lastFrame = value; if (value > firstFrameOfLastBlock) firstFrameOfLastBlock = value; } }
        public long LastFrame { get => lastFrame; internal set => lastFrame = value; }
        public long LastAddedBlockPos { get => lastAddedBlockPos; }

        public CoreDumpHeader(Int64 startIndex_a)// on besoin de savoir ou commence un block quand on le crée.
        {
            startPosition = startIndex_a;
            totalSize = headerSize;
            firstFrame = 0;
        }
        public bool IsCompressed()
        {
            return (configuration & (byte)config_flags.COMPRESSED) > 0;
        }

        public bool IsImportant()
        {
            return (configuration & (byte)config_flags.IMPORTANT) > 0;
        }

        public bool isBaseBlock()
        {
            return (configuration & (byte)config_flags.BASE) > 0;
        }
        public bool isExternFile()
        {
            return ((configuration) & (byte)config_flags.EXTERN_FILE)>0;
        }

        public void SetCompressed()
        {
            configuration |= (byte)config_flags.COMPRESSED;
        }
        public void SetExternFile()
        {
            configuration |= (byte)config_flags.EXTERN_FILE;
        }

        public void GoStartIndex(Stream st)
        {
            st.Seek(startPosition, SeekOrigin.Begin);
        }
        public void GoBlockEnd(Stream st)
        {
            st.Seek(BlockEnd(),SeekOrigin.Begin);
        }

        public void TerminateBlock(Stream st)
        {
            configuration |= (byte)config_flags.FINISHED;
            UpdateHeader(st);
        }
        protected void WriteHeader(Stream output)//écrie le header dans un stream : n'est pas responsable de la position dans le stream ou du fait que le stream soit inscriptible
        {
            byte[] bytes = BitConverter.GetBytes(totalSize);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(predBlockSize);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(predFramePerBlock);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(firstFrame);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(lastFrame);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(lastAddedBlockPos);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(firstFrameOfLastBlock);
            output.Write(bytes, 0, bytes.Length);

            bytes = BitConverter.GetBytes(configuration);
            output.Write(bytes, 0, 1);

        }
        public void UpdateHeader(Stream output)//ne chnage pas la position dans le stream
        {
            long c_pos = output.Position;
            GoStartIndex(output);
            WriteHeader(output);
            output.Seek(c_pos, SeekOrigin.Begin);//on remet la position au début
        }
        protected void ReadHeader(Stream input)//le stream est mis à la fin du header ( donc la fonction altère l'état du stream)
        {
            GoStartIndex(input);
            byte[] bytes = new byte[8];
            input.Read(bytes, 0, 8);
            totalSize = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            predBlockSize = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            predFramePerBlock = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            firstFrame = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            lastFrame = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            lastAddedBlockPos = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 8);
            firstFrameOfLastBlock = BitConverter.ToInt64(bytes, 0);
            input.Read(bytes, 0, 1);
            configuration = bytes[0];
        }

        //retourne la position du prochain block(si le header à le flags BASE alors il s'agit de la frame que l'on cherche)
        public long PredictFramePosition(long f, Stream st)
        {
            if ((configuration & (byte)config_flags.FINISHED) > 0)//si on a fini le block
            {
                if (predFramePerBlock!=0&&((f- firstFrame )/ predFramePerBlock == 0))
                {
                    return startPosition + headerSize;
                }else if (f > firstFrame && f < lastFrame)//last frame est bien la dernière frame du bloc
                {
                    if (f >= firstFrameOfLastBlock)
                    {//cas particulier

                        return startPosition + lastAddedBlockPos;
                    }
                    else//cas classique
                    {
                        long approx = startPosition + headerSize + ((predBlockSize) * ((f - firstFrame) / predFramePerBlock))-1;
                        long last_pos = st.Position;
                        byte[] b = new byte[1];
                        int i = 0;
                        st.Seek(approx, SeekOrigin.Begin);
                        do
                        {
                            st.Read(b, 0, 1);
                            i++;
                        } while (b[0] != MARK_CHAR && i < predBlockSize && st.Position<st.Length);//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
                        //TODO: possiblement erooné ou sur-limité
                        long pos = st.Position;
                        st.Seek(last_pos, SeekOrigin.Begin);//on remet en bonne position
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
                if (predFramePerBlock != 0 && (f-firstFrame) / predFramePerBlock == 0)
                {
                    return startPosition + headerSize;
                }
                else if (f >= firstFrame && f < lastFrame)//comme d'habitude
                {

                    long approx = startPosition + headerSize + ((predBlockSize) * ((f - firstFrame) / predFramePerBlock))-1;
                    long last_pos = st.Position;
                    byte[] b = new byte[1];
                    int i = 0;
                    st.Seek(approx, SeekOrigin.Begin);

                    do
                    {
                        st.Read(b, 0, 1);
                        i++;
                    } while (b[0] != MARK_CHAR && i < predBlockSize && st.Position < st.Length);//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
                                                                     //TODO: possiblement erooné ou sur-limité
                    long pos = st.Position;
                    st.Seek(last_pos, SeekOrigin.Begin);//on remet en bonne position
                    return pos;

                }
                else//on est dans le dernier block non comprimé
                {

                    return startPosition + TotalSize;
                }
            }
        }
        //retourne si la prédiction est fausse pour le prochain Block
        //TODO a verifier => pas sur (dépend de l'ajout pour savoir si il sera invalid )
        public bool PredictHit(Stream st)
        {
            long approx_fin = headerSize + (predBlockSize * ((lastFrame  -1- firstFrame) / predFramePerBlock));//approx de la prediction de la fin du block//
            long pred_fin = PredictFramePosition(LastFrame-1, st);
            long after_end = PredictFramePosition(LastFrame, st);//predicition du début
            long pred_debut = PredictFramePosition(firstFrameOfLastBlock, st);//predicition du début
            if (!(pred_debut  == (lastAddedBlockPos + StartPosition) && ((pred_fin == (lastAddedBlockPos + StartPosition) && after_end>TotalSize) || predFramePerBlock == 1)))
            {
                long raw_approx = startPosition + headerSize + (predBlockSize * ((firstFrameOfLastBlock - firstFrame) / predFramePerBlock));
                Console.WriteLine("echec de predicteur : r_a=" + raw_approx + " approx_fin=" + approx_fin + " pred=" + pred_debut +" after="+after_end+ "lastblock_pos=" + lastAddedBlockPos + ":real="+(lastAddedBlockPos+startPosition)+" fFrameofLastBlock=" + firstFrameOfLastBlock + " fperblock=" + predFramePerBlock + "predbsize=" + predBlockSize + " totalsize=" + totalSize + " sp=" + StartPosition);
            }
            return pred_debut  == (lastAddedBlockPos + StartPosition) && ((pred_fin == (lastAddedBlockPos + StartPosition) && after_end >TotalSize) || predFramePerBlock == 1);//on verifie qu'on aurra le bon résultat.
        }
        public long SearchBlockEnd(Stream st)
        {
            int i = 0;
            long last_pos = st.Position;
            byte b;
            do
            {
                b = (byte)st.ReadByte();
                i++;
            } while (b != MARK_CHAR && i < predBlockSize+1 && st.Position < st.Length);//on trouve le caractère de départ ou on dépasse la taille d'un bloc 
            long end_pos = st.Position-1;
            st.Seek(last_pos, SeekOrigin.Begin);
            return end_pos;
        }

        protected void addBlockSize(Int64 Pos, Int64 size, Int64 frameinblock_a, Int64 firstFrameOfBlock = -1)
        {//fonction qui met à jour les valeur pour un block avec une taille size et framePerBlock à l'interieur
            if (predBlockSize == 0)//initialisation
            {
                predBlockSize = size;
                predFramePerBlock = frameinblock_a;
                if (firstFrameOfBlock >= 0) firstFrame = firstFrameOfBlock;//utili pour les sous block qui ne peuvent pas avoir cette information
                lastFrame = firstFrame + frameinblock_a;
                firstFrameOfLastBlock = firstFrame;
            }
            else
            {

                if (firstFrameOfBlock >= 0)
                {
                    firstFrameOfLastBlock = firstFrameOfBlock;
                }
                else
                {
                    firstFrameOfLastBlock = lastFrame;//on déduit la première frame de la dernière du bloc d'avant.
                }
                lastFrame += frameinblock_a; ;
            }
            totalSize += size;
            lastAddedBlockPos = Pos - StartPosition;//pos se repère par rapport a startpostion
            lastAddedBlockSize = size;
        }
        public Int64 BlockEnd()
        {
            return startPosition + totalSize;
        }
        public void BlockMarker(Stream st)
        {
            st.WriteByte(MARK_CHAR);
        }
        public bool isFinished()
        {
            return (configuration & (byte)config_flags.FINISHED)>0;
        }

        public void Dispose()
        {

            ;
        }
    }
}