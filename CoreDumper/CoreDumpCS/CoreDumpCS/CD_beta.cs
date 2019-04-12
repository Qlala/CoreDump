
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using SevenZip;

namespace ConsoleApp2
{
    
    abstract class CoreDumpFile: IDisposable
    {
        internal const long BLOCKCOUNT_OFFSET = 12;
        protected Int64 headerSize = 28;// en Int32;
        protected long currentBlockCount;
        protected int blockSize=0;
        protected UInt64 configuration=0;

        //description du header(unité en byte)
        // |   HeaderSize(8)    |
        // |    Bloc Size (4)  |
        // |   Bloc Count(8)   |
        //|  Configuration(8)  |
        //|coder Properties (?)|


        protected FileStream fStream;
        protected bool valid = false;
        private string filepath ;
        protected virtual void WriteCoderProperties(Stream st) { }
        protected virtual void ReadCoderProperties(Stream st) {; }
        public void WriteHeader()
        {
            if (fStream.CanWrite)
            {
                fStream.Seek(0, SeekOrigin.Begin);
                byte[] bytes= BitConverter.GetBytes(headerSize);
                fStream.Write(bytes, 0, bytes.Length);
                bytes = BitConverter.GetBytes(blockSize);
                fStream.Write(bytes, 0, bytes.Length);
                bytes = BitConverter.GetBytes(currentBlockCount);
                fStream.Write(bytes, 0, bytes.Length);
                bytes = BitConverter.GetBytes(configuration);
                fStream.Write(bytes, 0, bytes.Length);
                WriteCoderProperties(fStream);
                headerSize = (fStream.Position>headerSize)?fStream.Position:headerSize;//on n'overwrite pas les données du coder permet  de laissr la liberté a l'agorithme de compression pour faire un header plus long
                fStream.Seek(0, SeekOrigin.Begin);
                bytes = BitConverter.GetBytes(headerSize);
                fStream.Write(bytes, 0, bytes.Length);

                System.Console.Out.WriteLine("Header Lu avec "+currentBlockCount.ToString()+"block");
                System.Console.Out.WriteLine("bsize=" + blockSize.ToString()) ;

            }
        }


        public void ReadHeader()
        {
            if (fStream.CanRead)
            {
                fStream.Seek(0, SeekOrigin.Begin);
                byte[] bytes=new byte[8];
                fStream.Read(bytes, 0, 8);
                headerSize=BitConverter.ToInt64(bytes, 0);
                fStream.Read(bytes, 0, 4);
                blockSize = BitConverter.ToInt32(bytes, 0);
                fStream.Read(bytes, 0, 4);
                currentBlockCount = BitConverter.ToInt64(bytes, 0);
                fStream.Read(bytes, 0, 4);
                configuration = BitConverter.ToUInt64(bytes, 0);
                ReadCoderProperties(fStream);//on profite pour recupérer les donner du coder
                System.Console.Out.WriteLine("Header Lu avec"+currentBlockCount.ToString() + " block de " + blockSize.ToString());
            }
        }

        public bool loadFile(string file_path,FileAccess fa = FileAccess.ReadWrite)//retourne vrai si le fichier est  nouveau
        {
            if (File.Exists(file_path))//ouverture en lecture et ecriture
            {
                fStream = new FileStream(file_path, FileMode.Open,fa|FileAccess.Read) ;//forcément au moins en lecture
                valid = true;
                return false;
            }
            else//creation du fichier
            {
                fStream = new FileStream(file_path, FileMode.Create, FileAccess.Write|fa);//forcément au moins en écriture
                valid = fStream.CanWrite;
                return true;//nouveau fichier
            }
        }

        public void UpdateBlockCount()//met à jour uniquement le block count dans le fichier
        {
            if (fStream.CanWrite)
            {
                long lp=fStream.Position;//on retiens l'ancienn eposition
                byte[] bytes = BitConverter.GetBytes(headerSize);
                bytes = BitConverter.GetBytes(currentBlockCount);
                fStream.Seek(BLOCKCOUNT_OFFSET, SeekOrigin.Begin);
                fStream.Write(bytes, 0, bytes.Length);
                fStream.Seek(lp, SeekOrigin.Begin); //on restore l'ancienne position
            }
        }

        public void SeekBlocksStart(Int64 block_nb=0)
        {
  
            if (fStream.CanRead) {
                fStream.Seek(headerSize+block_nb*blockSize, SeekOrigin.Begin);
             }
        }
        public void seekBlocksEnd()
        {
            fStream.Seek(0, SeekOrigin.End);
        }
        public void Dispose()
        {
            fStream.Close();
        }
        ~CoreDumpFile()
        {
            Dispose();
        }

    }
    interface IBlockEncode//defini les requistition pour les compression des block
    {
        bool Encode(Stream data, Stream output,Int64 length);
    }
    interface IBlockDecode//defini les requistition pour les compression des block
    {
        bool Decode(Stream data, Stream output);
    }
    interface IReadCoderProperties
    {
        void ReadCoderProperties(Stream st);
    }

    abstract class LZMA_implementation //contiens les paramètre pour l'implémentation du LZMA par defaut
    {
       protected CoderPropID[] PropIDs =
        {
                            CoderPropID.DictionarySize,
                            CoderPropID.PosStateBits,
                            CoderPropID.LitContextBits,
                            CoderPropID.LitPosBits,
                            CoderPropID.Algorithm,
                            CoderPropID.NumFastBytes,
                            CoderPropID.MatchFinder,
                            CoderPropID.EndMarker
        };
        protected object[] default_properties()
        {
            const Int32 dictionary = 1 << 23;

            const Int32 posStateBits = 2;
            const Int32 litContextBits = 3; // for normal files
                                            // UInt32 litContextBits = 0; // for 32-bit data
            const Int32 litPosBits = 0;
            // UInt32 litPosBits = 2; // for 32-bit data
            const Int32 algorithm = 2;
            const Int32 numFastBytes = 128;
            const string mf = "bt4";//match finder
            const bool eos = true;
            object[] properties =
            {
                (Int32)(dictionary),
                            (Int32)(posStateBits),
                            (Int32)(litContextBits),
                            (Int32)(litPosBits),
                            (Int32)(algorithm),
                            (Int32)(numFastBytes),
                            mf,
                            eos
            };
            return properties;
        }

    }


    class LZMA_Encoder : LZMA_implementation,IBlockEncode,IReadCoderProperties,IWriteCoderProperties
    {
        private SevenZip.Compression.LZMA.Encoder encoder;
        private bool properties_set = false;

        public void WriteCoderProperties(Stream output)
        {
            if (!properties_set) {
                encoder.SetCoderProperties(PropIDs, default_properties());
             }
            encoder.WriteCoderProperties(output);
        }
        static internal T Deserialize<T>(byte[] param)
        {
            using (MemoryStream ms = new MemoryStream(param))
            {
                IFormatter br = new BinaryFormatter();
                return (T)br.Deserialize(ms);
            }
        }
        public void ReadCoderProperties(Stream st)
        {
            byte[] properties = new byte[5];
            if (st.Read(properties, 0, 5) == 5) {
                encoder.SetCoderProperties(PropIDs, Deserialize<object[]>(properties));
                properties_set = true;
            }
            else
            {
                System.Console.Out.WriteLine("header de l'encodage incomplet");
            }
        }

        public bool Encode(Stream data, Stream output,Int64 length)
        {
            if (properties_set)
            {
                encoder.Code(data, output, length, -1, null);
                return true;
            }
            else
            {
                return false;
            }
           
        }

        public LZMA_Encoder()
        {
            encoder = new SevenZip.Compression.LZMA.Encoder();
        }

    }

    class LZMA_Decoder : LZMA_implementation, IBlockDecode,IReadCoderProperties
    {
        private SevenZip.Compression.LZMA.Decoder decoder;
        private bool properties_set = false;

        public void ReadCoderProperties(Stream st)
        {
            byte[] properties = new byte[5];
            st.Read(properties, 0, 5);
            decoder.SetDecoderProperties(properties) ;
            properties_set = true;
        }

        public bool Decode(Stream data, Stream output)
        {
            if (properties_set)
            {
                decoder.Code(data, output, -1, -1, null);
                return true;
            }
            else
            {
                return false;
            }

        }

        public LZMA_Decoder()
        {
            decoder = new SevenZip.Compression.LZMA.Decoder();
        }

    }



    class CoreDumper<T>:  CoreDumpFile where T : IBlockEncode,IWriteCoderProperties,IReadCoderProperties,new() {//T est un encoder

        //TODO diférencier propriété et champ



        //propriété de l'encoder

        T encoder;
        
        

        //encodeur et stream
        private MemoryStream lastFrame;
        override protected void WriteCoderProperties(Stream st)
        {
            encoder.WriteCoderProperties(st);
        }
        override protected void ReadCoderProperties(Stream st)
        {
            encoder.ReadCoderProperties(st);
        }
        public CoreDumper(string file_path,Int32 FrameSize=0)
        {
            encoder = new T();
            if (loadFile(file_path))
            {
                //nouveau fichier;
                blockSize = FrameSize;
                WriteHeader();
            }
            else
            {
                //ancien fichier
                ReadHeader();
                
            };

        }

        public void addFrame(Stream st)
        {
            if (fStream.CanWrite)
            {
                if (blockSize == 0)//auto set block size
                {
                    blockSize =(Int32)st.Length;
                    WriteHeader();
                }
                seekBlocksEnd();
                encoder.Encode(st, fStream, blockSize);
                currentBlockCount += 1;
                UpdateBlockCount();

            }
        }
        public void Dispose() {
            //ne fait rien (permet juste l'usage des forme using
        }
        ~CoreDumper()
        {
            Dispose();
        }
    }

    class FramExtracter<T> : CoreDumpFile where T : IBlockDecode, IReadCoderProperties, new()
    {//T est un encoder

        //TODO diférencier propriété et champ



        //propriété de l'encoder

        T decoder;



        //encodeur et stream
        private MemoryStream lastFrame;
        override protected void ReadCoderProperties(Stream st)
        {
            decoder.ReadCoderProperties(st);
        }


        public FramExtracter(string file_path, UInt32 FrameSize = 0)
        {
            decoder = new T();
            if (!loadFile(file_path, FileAccess.Read))
            {
                ReadHeader();
            }
            else
            {
                valid = false;//invalidation forcé fichier inexistant;

            };

        }
        public MemoryStream RetrieveFrame(uint N)
        {
            if (N < currentBlockCount)
            {
                MemoryStream temp = new MemoryStream(blockSize);
                SeekBlocksStart(N);
                decoder.Decode(fStream, temp);
                return temp;
            }
            else
            {
                return null;
            }
        }
        public void Dispose()
        {
            //ne fait rien (permet juste l'usage des forme using
        }
    }
        
}
