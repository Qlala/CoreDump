using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using SevenZip;
using System.Runtime;
using Fossil;
using System.IO.Compression;
using System.Diagnostics;
namespace CoreDumper
{
    abstract class CoreDumpFileTop
    {
        protected Stream output;
        int cnt;
        public Stream Output { get { return output; } }
        virtual public bool EncodingNeeded(int depth) { return true; }
        virtual public bool MaxBlockCountReach(int depth, int count) { return false; }
        virtual public bool SeparateFileNeeded(int depth) { return false; }
        virtual public string SeperateFileName(int depth,int nb)
        {
            cnt++;
            return "test." + cnt +".part";
        }
        virtual public bool Encode(Stream input, Stream output, Int64 in_l, Int64 out_l) { return false; }
        virtual public bool Decode(Stream input, Stream output, Int64 in_l, Int64 out_l) { return false; }
        virtual public bool Named_Decode(String name, Stream input, Stream output, Int64 in_l, Int64 out_l) { return false; }

        internal BlockType tree;
        public BlockType CreateNewBlock(int depth, Stream source, long first_frame,bool no_write = false)
        {
            if (depth < 1)
            {
                return new BlockLeaf(this, source, first_frame, no_write);
            } else return CreateNewBlockTree(depth,source,first_frame,no_write);
        }
        public BlockTree CreateNewBlockTree(int depth, Stream source, long first_frame, bool no_write = false)
        {
            if (SeparateFileNeeded(depth))
            {
                return new BlockFileTree(this, source, depth, first_frame, no_write);
            }
            else
            {
                return new BlockTree(this, source, depth, first_frame, no_write);
            }
        }
        public Int64 RetrieveFrameCount()
        {
            Int64 lastframe=tree.FirstFrame;
            tree.RetriveFrameCount(Output, ref lastframe);
            return lastframe;
        }
        public Int64 RetrieveFirstFrame()
        {
            return tree.FirstFrame;
        }

        virtual public void AddFrame(Stream st)//TODO => améliorable
        {
            if (tree.AddFrame(st))//si renvoie vrais alors l'arbre est plein et on sait que l'on est à la fin des zone écrite (fin des blocks)
            {
                RebaseTree();
                
            }//pas de problème sinon
        }
        private void RebaseTree()
        {
            tree.GoStartIndex(Output);
            BlockTree temp = CreateNewBlockTree(tree.Depth + 1, Output, 0, true);
            //BlockTree temp = new BlockTree(this, Output, tree.Depth + 1, 0, true);

            temp.addChildBlock(tree, Output, true);//on lui ajoute le block de l'arbre précedent
            
            //le Stream est donc bien mis juste après le dernire block ajouté
            long f_child = temp.LastAddedBlockPos;
            temp.CreateNewChild(Output);//on lui recrée ces enfants

            System.Console.WriteLine(Output.Position);
            System.Console.WriteLine("on a rebasé l'arbre et le nouvelle enfant est en " + temp.TotalSize + "et le premier est en :" + f_child + "et contient les frame de" + tree.FirstFrame + "à" + tree.LastFrame);
            tree = temp;//on lui fait toute suite crée un nouvelle enfant : le prédicteur est forcément valide.
            Output.Flush();
            output.SetLength(tree.TotalSize);
            tree.UpdateHeader(Output);//on met à jour le header de l'arbre
        }

        public void FinishTree()
        {
            tree.FinishTree(Output);
            RebaseTree();
        }
        public void ReadOnlyRestore(Stream fst)
        {
            if (fst.CanRead)
            {
                output = fst;
                tree = new BlockType(this, Output.Position, 0);
                tree.UpdateHeaderFromFile();
            }
        }
        public virtual Stream DirectRetriveFrame(long f,int outputsize)
        {
            return tree.RetrieveFrame(f, Output, outputsize, true);
        }
        public virtual byte[] randomAccesFrame(long f,long pos,long size)
        {
            List<byte> a=new List<byte>();
            int outputsize=0;
            Stream frame = DirectRetriveFrame(f, outputsize);
            frame.Seek(pos, SeekOrigin.Current);
            for (int i = 0; i < size; i++) a.Add((byte)frame.ReadByte());
            return a.ToArray();
        }

        public virtual Stream RetriveFrame(long f)
        {
            return tree.RetrieveFrame(f,Output,0,false);
        }
        public long find_lastframe()
        {
            return 0;  
        }

        public void RestoreTree(Stream fst)
        {
            if (fst.CanRead)//on vérifie que l'on peut exploiter le fichier
            {
                output = fst;
                long st_pos = fst.Position;
                BlockType temp = new BlockType(this, st_pos,0);
                temp.UpdateHeaderFromFile();//conserve la position du stream
                if (temp.isBaseBlock())//on a un block de base
                {
                    tree = new BlockLeaf(this, fst,0, true);
                    tree.UpdateHeaderFromFile();
                }
                else//on a un arbre
                {
                    BlockTree t_tree = new BlockTree(this, fst, 1,0,true);
                    tree = t_tree;
                    tree.UpdateHeaderFromFile();
                    //on restore récursivement
                    t_tree.RestoreTree();
                }

            }
        }
    }
    interface ICode
    {
        void Code(System.IO.Stream inStream, System.IO.Stream outStream,
            Int64 inSize, Int64 outSize, ICodeProgress progress);
    }


    //version plus simple que le CoreDumpWrite (contient le fastdeccode deccomprssion
    class CoreDumpOpener<U> : CoreDumpFileTop, IDisposable where U : ICode
    {
        Dictionary<String, Stream> fastdecode_compression_stream=new Dictionary<string, Stream>();
        Dictionary<String, long> fastdecode_compression_size=new Dictionary<string, long>();
        U decoder;
        String CurrentFileName;
        public U Decoder { get => decoder; set => decoder = value; }

        override public bool Decode(Stream input, Stream output, Int64 in_l, Int64 out_l)
        {
            decoder.Code(input, output, in_l, out_l, null);
            return true;
        }
        override public bool Named_Decode(String name, Stream input, Stream output, Int64 in_l, Int64 out_l)
        {
            if (fastdecode_compression_stream.ContainsKey(name))
            {
                Console.WriteLine("decodage depuis la sauvegarde");
                fastdecode_compression_stream[name].Seek(0, SeekOrigin.Begin);
                fastdecode_compression_stream[name].CopyTo(output);
                output.Seek(0, SeekOrigin.Begin);
                out_l =fastdecode_compression_size[name];
            }
            else
            {
                if (fastdecode_compression_stream.Count > 3)
                {
                    KeyValuePair<string, Stream> n_to_remove = fastdecode_compression_stream.First<KeyValuePair<string, Stream>>();
                    fastdecode_compression_stream.Remove(n_to_remove.Key);
                    fastdecode_compression_size.Remove(n_to_remove.Key);
                }
                Console.WriteLine("sauvegarde du decode=" + name);
                decoder.Code(input, output, in_l, out_l, null);
                MemoryStream temp = new MemoryStream();
                output.Seek(0, SeekOrigin.Begin);
                output.CopyTo(temp);
                fastdecode_compression_stream.Add(name, temp);
                fastdecode_compression_size.Add(name, out_l);

            }
            
            return true;
        }

        public override void AddFrame(Stream st)
        {
            Console.WriteLine("AddFrame is Disabled");
        }

        public void readOnlyOpen(string path)
        {
            CurrentFileName = path;
            Stream fst = new FileStream(path, FileMode.Open, FileAccess.Read);
            ReadOnlyRestore(fst);
        }

        public void Dispose()
        {
            output.Dispose();
        }
        public CoreDumpOpener(U d)
        {
            decoder = d;
        }
    }

        class CoreDumpWriter<T,U> : CoreDumpFileTop,IDisposable where T : ICode where U:ICode

    {
        T coder;
        U decoder;
        String CurrentFileName;

        int Encoding_min_depth = 2;
        int Encoding_max_depth = 3;
        int TreeFileSeparationLevel = 2;

        int max_block = 10;

        public T Coder { get => coder; set => coder = value; }
        public U Decoder { get => decoder; set => decoder = value; }

        override public bool EncodingNeeded(int depth) {
            return ((depth>=Encoding_min_depth && depth<Encoding_max_depth));
        }
        override public bool MaxBlockCountReach(int depth, int count)
        {
            return count>=max_block;
        }
        public override bool SeparateFileNeeded(int depth)
        {
            return depth== TreeFileSeparationLevel;
        }
        int cnt;
        public override string SeperateFileName(int depth, int nb)
        {
            if (CurrentFileName.LastIndexOf("/") > 0)
            {
                cnt++;
                String temp = CurrentFileName.Substring(0, CurrentFileName.LastIndexOf("/"));
                String temp2 = CurrentFileName.Substring(CurrentFileName.LastIndexOf("/"));
                String StripExtension = temp + "/" + temp2.Substring(0, temp2.IndexOf("."));
                if (!Directory.Exists(StripExtension)) Directory.CreateDirectory(StripExtension);

                return StripExtension + "/" + cnt + ".part";
            }
            else
            {
                cnt++;
                String StripExtension =CurrentFileName.Substring(0, CurrentFileName.IndexOf("."));
                if (!Directory.Exists(StripExtension)) Directory.CreateDirectory(StripExtension);

                return StripExtension + "/" + cnt + ".part";
            }
        }
        class progresshandler:ICodeProgress
        {
            long size;
            long last;
            public progresshandler(long s)
            {
                size = s;
            }
            public void SetProgress(long inSize, long outSize)
            {
                if ((inSize / 100000) != last)
                {
                    
                    Console.WriteLine("Pogression:" + (inSize / 100000).ToString() + "/" + (size / 100000).ToString());
                    Console.CursorTop -= 1;
                    last = (inSize / 100000);

                }
            }
        }

        override public bool Encode(Stream input, Stream output, Int64 in_l, Int64 out_l) {
            Console.WriteLine("encodage en cours");
            progresshandler ph = new progresshandler(input.Length);
            coder.Code(input,output,in_l,out_l,ph);
            return true;
        }

        override public bool Decode(Stream input, Stream output, Int64 in_l, Int64 out_l)
        {
                decoder.Code(input, output, in_l, out_l, null);
                return true;
        }
        override public bool Named_Decode(String name,Stream input, Stream output, Int64 in_l, Int64 out_l)
        {
            decoder.Code(input, output, in_l, out_l, null);
            return true;
        }

        public override void AddFrame(Stream st)
        {
            base.AddFrame(st);
        }

        public void Open(string path,FileAccess fa)
        {
            if (File.Exists(path))
            {
                if (((byte)fa | (byte)FileAccess.Write) > 0)
                {
                    OpenExistingFile(path);
                }
                else
                {
                    readOnlyOpen(path);
                }
            }
            else
            {
                OpenNewFile(path);
            }
        }
        public CoreDumpWriter(T e,U d)
        {
            coder = e;
            decoder = d;
        } 
        public CoreDumpWriter()
        {

        }

        public void OpenNewFile(string path)
        {
            output = new FileStream(path, FileMode.CreateNew,FileAccess.ReadWrite);
            tree = new BlockLeaf(this, output);
            tree.FirstFrame = 0;
            CurrentFileName = path;
        }
        public void OpenExistingFile(string path)
        {
            CurrentFileName = path;
            Stream fst = new FileStream(path, FileMode.Open, FileAccess.ReadWrite);
            RestoreTree(fst);
        }
        public void readOnlyOpen(string path)
        {
            CurrentFileName = path;
            Stream fst = new FileStream(path, FileMode.Open, FileAccess.Read);
            ReadOnlyRestore(fst);
        }

        public void Dispose()
        {
            output.Dispose();
        }

    }

    

}
