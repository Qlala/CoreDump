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
    interface IAddFrame
    {
        void AddFrame(Stream st);//avertie le coder qu'une frame a été ajouté (il en fait ce qu'il veut)
    }





    class CoreDumpWriter<T,U> : CoreDumpFileTop,IDisposable where T : ICode,IAddFrame where U:ICode

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
            //try
            //{
                decoder.Code(input, output, in_l, out_l, null);
                return true;
            /*}
            //catch (Exception e)
            //{
                throw new Exception();
                return false;
            }*/
        }

        public override void AddFrame(Stream st)
        {
            base.AddFrame(st);
            coder.AddFrame(st);//permet au coder a delta de savoir a quelle frame il se réference
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

    class CoreDumpDeltaWriter<T,U> : CoreDumpWriter<T, U> where T:ICode,IAddFrame where U:ICode
    {
        static long DELTA_TH=50000;
        MemoryStream current_reference=new MemoryStream();

        long reference_id = -1;
        long last_added_frame = -1;

        Dictionary<long, Stream> fastDecode_keptFrame=new Dictionary<long, Stream>();
        //Stream fastDecode_keptframe;
        Dictionary<long, long> fastDecode_keptFrame_pos=new Dictionary<long, long>();
        public Stream ComputeDelta(Stream st,long f) {


            Stopwatch sw = new Stopwatch();
            st.Seek(0, SeekOrigin.Begin);
            sw.Start();
            byte[] in_buff = new byte[st.Length];
           
            st.Read(in_buff,0, (int)st.Length);
            sw.Stop();
            Console.WriteLine("temps copie entré:" + sw.Elapsed);
            sw.Reset();
            sw.Start();
            byte[] out_buff = Fossil.Delta.Create(current_reference.GetBuffer(), in_buff);
            sw.Stop();
            Console.WriteLine("calcul delta =>" + sw.Elapsed);
            Stream out_st = new MemoryStream();
            sw.Reset();
            sw.Start();
            out_st.WriteByte(1);
            out_st.Write(BitConverter.GetBytes((int)(f - reference_id)),0,4);
            out_st.Write(out_buff, 0, out_buff.Length);
            out_st.Seek(0, SeekOrigin.Begin);
            sw.Stop();
            Console.WriteLine("temps copie sortie:" + sw.Elapsed);
            return out_st;
        }
        Stream MakeNewReference(Stream st,long f)
        {
            current_reference.Dispose();
            current_reference = new MemoryStream();
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(current_reference);
            reference_id = f;
            Stream temp = new MemoryStream(1);
            temp.WriteByte(0);
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(temp);
            temp.Seek(0, SeekOrigin.Begin);
            return temp;
        }
        void KeepFrame(Stream kept_frame,long f)
        {
            Console.WriteLine("Sauvegarde de la frame:" + f);
            fastDecode_keptFrame.Add(f,kept_frame);
            fastDecode_keptFrame_pos.Add(f, kept_frame.Position);
            KeyValuePair<long, Stream>  f_to_remove =fastDecode_keptFrame.First<KeyValuePair<long, Stream>>();
            fastDecode_keptFrame.Remove(f_to_remove.Key);
            fastDecode_keptFrame_pos.Remove(f_to_remove.Key);
            /*fastDecode_keptframe_pos = kept_frame.Position;
            fastDecode_keptframe = kept_frame;
            fastDecode_keptframe_id = f;*/
        }

        Stream SearchOriginalFrame(long f,int outputsize)
        {
            if (fastDecode_keptFrame.ContainsKey(f))
            {
                Console.WriteLine("fast deccord frame:" +f);
                fastDecode_keptFrame[f].Seek(fastDecode_keptFrame_pos[f], SeekOrigin.Begin);
                return fastDecode_keptFrame[f];
            }
            else
            {
                Stream frame=DirectRetriveFrame(f, outputsize);
                KeepFrame(frame, f);
                return frame;
            }
        }

        public Stream CodeDelta(Stream frame)
        {
            last_added_frame++;
            if (reference_id == -1)
            {
                return MakeNewReference(frame, last_added_frame);
            }
            else
            {
                Stream delta = ComputeDelta(frame,last_added_frame);
                if (delta.Length > DELTA_TH)
                {
                    Console.WriteLine("Delta trop grand :" + delta.Length);
                    return MakeNewReference(frame, last_added_frame);
                }
                else
                {
                    
                    return delta;
                }
            }
            
        }
        int DELTA_WINDOW_SIZE =1024;
        public MemoryStream ApplyDelta(Stream delta,Stream source)
        {
            MemoryStream output = new MemoryStream();
            byte[] buff = new byte[1024];
            while (delta.Position<delta.Length)
            {
                
                if (delta.ReadByte() == 'C')
                {
                    source.Read(buff, 0, DELTA_WINDOW_SIZE);
                    output.Write(buff, 0, DELTA_WINDOW_SIZE);
                }
                else//P
                {
                    delta.Read(buff, 0, DELTA_WINDOW_SIZE);
                    source.Position += DELTA_WINDOW_SIZE;
                    output.Write(buff, 0, DELTA_WINDOW_SIZE);
                }
            }
            return output;
        }
        public byte[] RA_ApplyDelta(long pos,long size,Stream delta, Stream source)
        {
            byte[] output = new byte[size];
            long approx_pos = (pos / DELTA_WINDOW_SIZE)*DELTA_WINDOW_SIZE;
            long data_copied = 0;
            long curr_pos=0;
            byte[] buff = new byte[DELTA_WINDOW_SIZE];
            while (curr_pos < approx_pos)
                {
                    if (delta.ReadByte() == 'C')
                    {
                        curr_pos += DELTA_WINDOW_SIZE;
                    }
                    else
                    {
                        delta.Position += DELTA_WINDOW_SIZE;
                        curr_pos += DELTA_WINDOW_SIZE;
                    }
                }
            
            source.Position+=curr_pos;
            if (curr_pos < pos )
            {
               
                {
                    if (delta.ReadByte() == 'C')
                    {
                        source.Read(buff, 0, DELTA_WINDOW_SIZE);
                    }
                    else//P
                    {
                        delta.Read(buff, 0, DELTA_WINDOW_SIZE);
                        source.Position += DELTA_WINDOW_SIZE;
                    }
                    buff.Skip((int)(pos - curr_pos)).Take((int)size).ToArray().CopyTo(output, 0);
                    data_copied += size;
                }

            }

            while (data_copied+DELTA_WINDOW_SIZE < size)
            {
                if (delta.ReadByte() == 'C')
                {
                    source.Read(buff, 0, DELTA_WINDOW_SIZE);
                }
                else//P
                {
                    delta.Read(buff, 0, DELTA_WINDOW_SIZE);
                    source.Position += DELTA_WINDOW_SIZE;
                }
                buff.CopyTo(output,data_copied);
                data_copied += DELTA_WINDOW_SIZE;
            }
            buff = new byte[size - data_copied];
            if (delta.ReadByte() == 'C')
            {
                source.Read(buff, 0, (int)(size-data_copied));
            }
            else//P
            {
                delta.Read(buff, 0, (int)(size - data_copied));
            }
            buff.CopyTo(output, data_copied);
            return output.ToArray();
        }
        public MemoryStream ResolveDelta(Stream st,long f)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize=0;
            Stream ref_frame = SearchOriginalFrame(f-delta_ref_f,outputsize);
            ref_frame.ReadByte();
            return ApplyDelta(st,ref_frame);


        }
        public byte[] DA_ResolveDelta(Stream st, long f,long pos,long size)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize = 0;
            Stream ref_frame = SearchOriginalFrame(f - delta_ref_f, outputsize);
            ref_frame.ReadByte();
            //return Fossil.Delta.RA_ApplyStream(pos,size,ref_frame, st);
            return RA_ApplyDelta(pos, size, st,ref_frame);

        }


        public override byte[] randomAccesFrame(long f, long pos, long size)
        {
            Console.WriteLine("random acces : frame=" + f);
            Stopwatch sw = new Stopwatch();
            sw.Start();
            int outputsizea = 0;
            Stream temp = DirectRetriveFrame(f, outputsizea);
            sw.Stop();
            Console.WriteLine("temps recuperation delta:" + sw.Elapsed);
            sw.Reset();
            //temp.Seek(0, SeekOrigin.Begin);
            if (temp.ReadByte() == 0)
            {
                List<byte> a = new List<byte>();
                temp.Seek(pos, SeekOrigin.Current);
                for (int i = 0; i < size; i++) a.Add((byte)temp.ReadByte());
                return a.ToArray();
            }
            else
            {
                return DA_ResolveDelta(temp,f,pos,size);
            }
        }

        public override Stream RetriveFrame(long f)
        {
            Stopwatch sw = new Stopwatch();
            sw.Start();
            Stream temp = base.RetriveFrame(f);
            sw.Stop();
            Console.WriteLine("temps recuperation delta:" + sw.Elapsed);
            sw.Reset();
            temp.Seek(0, SeekOrigin.Begin);
            if (temp.ReadByte() == 0)
            {
                MemoryStream temp2 = new MemoryStream();
                temp.CopyTo(temp2);
                temp.Dispose();
                temp2.Seek(0, SeekOrigin.Begin);
                return temp2;
            }
            else
            {
                return ResolveDelta(temp, f);
            }
            
        }

        public override void AddFrame(Stream st)
        {
            base.AddFrame(CodeDelta(st));
        }
    }
    

}
