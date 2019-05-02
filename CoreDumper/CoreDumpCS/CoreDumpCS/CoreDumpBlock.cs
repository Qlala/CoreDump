using System;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace CoreDumper
{

    interface IBlockInterface
    {
        bool AddFrame(Stream st);//renvoie true quand le regroupement de bloc est remplis
        void FinishTree(Stream source);

    }
    class BlockType : CoreDumpHeader, IBlockInterface
    {
        CoreDumpFileTop top;
        Stream fileSource;
        protected int blockCount = 0;
        internal CoreDumpFileTop Top { get => top; }
        public virtual int Depth { get => 0; }
        protected Stream FileSource { get => fileSource; set => fileSource = value; }

        public BlockType(CoreDumpFileTop top_a, Int64 start,long first_frame) : base(start)
        {
            FirstFrame = first_frame;
            top = top_a;
            FileSource = top.Output;//defaultFileSource
        }

        public void UpdateHeaderFromFile()
        {
            long c_pos = top.Output.Position;
            GoStartIndex(top.Output);
            ReadHeader(top.Output);
            top.Output.Seek(c_pos, SeekOrigin.Begin);//restore stream position
        }

        virtual public bool AddFrame(Stream st) { return true; }
        virtual public void FinishTree(Stream source)
        {
            ;
        }
        public void RetriveFrameCount(Stream st,ref Int64 foundLastFrame)
        {
            if (foundLastFrame > FirstFrame)
            {
                Console.WriteLine("block bugé");
                return;//bloc bugé
            }
            else
            {
                foundLastFrame = foundLastFrame > LastFrame ? foundLastFrame : LastFrame;
                Console.WriteLine("FoundlastFrame="+foundLastFrame);
                try
                {
                    BlockType temp = LoadBlockAt(ref st, BlockEnd());
                    temp.RetriveFrameCount(st, ref foundLastFrame);
                }
                catch(Exception e)
                {
                    Console.Write("Erreur d'accès");
                    return;//erreur pas de block ici
                }
                return;
            }
        }

        private BlockType LoadBlockAt(ref Stream st,long pred)
        {
            Console.WriteLine("Chargement du block a la position " + pred);
            if (pred >= 0 && pred < st.Length)
            {
                Int64 ori_pred = pred;
                st.Seek(pred, SeekOrigin.Begin);
                if (isExternFile())
                {
                    long end_pos = SearchBlockEnd(st);
                    st.Seek(pred, SeekOrigin.Begin);
                    int size = (int)(end_pos - pred);
                    byte[] buff = new byte[size];
                    st.Read(buff, 0, (size));
                    Console.WriteLine("fichier différent :" + Encoding.ASCII.GetString(buff));
                    st = new FileStream(Encoding.ASCII.GetString(buff), FileMode.Open, FileAccess.Read);
                    pred = 0;
                }
                {
                    if (!isBaseBlock())
                    {
                        if (ori_pred == TotalSize + StartPosition)//partie non fini
                        {

                            BlockType temp = new BlockType(top, pred, 0);

                            temp.ReadHeader(st);//on lis le header
                            return temp;

                        }
                        else if (IsCompressed())//Le fichier est compressé
                        {
                            Stopwatch sw = new Stopwatch();
                            sw.Start();
                            Console.WriteLine("Niveau intermédiare compréssé");
                            MemoryStream temp_st_2 = new MemoryStream();
                            Top.Decode(st, temp_st_2, -1, -1);
                            //on déccode le block
                            BlockType temp = new BlockType(top, 0, 0);
                            temp_st_2.Seek(0, SeekOrigin.Begin);
                            sw.Stop();
                            Console.WriteLine("deccodage en:" + sw.Elapsed);
                            temp.ReadHeader(temp_st_2);//on lis le header de depuis le stream qui à été décompréssé
                            return temp;
                        }
                        else
                        {//on est pas compressé donc on peut se contenter d'ouvrir le steam au bonne endroit

                            BlockType temp = new BlockType(top, pred, 0);

                            temp.ReadHeader(st);//on lis le header
                            return temp;
                        }

                    }
                    else
                    {
                        throw new Exception();
                    }
                }
            }
            else
            {
                Console.WriteLine("erreur base block");
                throw new Exception();
                //return new BlockType(); ;//erreur on retourne un steam vide
            }
        }
    


        public Stream RetrieveFrame(long f, Stream st,long outputSize,bool DirectAcces=false)
        {
            Console.WriteLine("bloc fini :" +isFinished()+" et block feuille:"+isBaseBlock());
            long pred = PredictFramePosition(f, st);
            if(predFramePerBlock>0) Console.WriteLine("frame prédite :" + (f / predFramePerBlock)* predFramePerBlock + " : Pos=" + pred +"(entre "+ FirstFrame+"et"+LastFrame +")");
            else Console.WriteLine("frame prédite :erreur" + " : Pos=" + pred + "(entre " + FirstFrame + "et" + LastFrame + ")");
            long ori_pred = pred;
            if (pred >= 0 && pred<st.Length)
            {
                st.Seek(pred, SeekOrigin.Begin);
                Stream tmp_temp_st = st;
                if (isExternFile())
                {
                    long end_pos = SearchBlockEnd(st);
                    st.Seek(pred, SeekOrigin.Begin);
                    int size=(int)(end_pos - pred);
                    byte[] buff = new byte[size];
                    st.Read(buff, 0, (size));
                    Console.WriteLine("fichier différent :" + Encoding.ASCII.GetString(buff));
                    tmp_temp_st = new FileStream(Encoding.ASCII.GetString(buff), FileMode.Open, FileAccess.Read); 
                    pred = 0;
                }
                Stream temp_st = tmp_temp_st;
                
                {
                    if (!isBaseBlock())
                    {
                        if (ori_pred >= TotalSize + StartPosition)//partie non fini
                        {

                            BlockType temp = new BlockType(top, pred, 0);

                            temp.ReadHeader(temp_st);//on lis le header
                            return temp.RetrieveFrame(f, temp_st,outputSize,DirectAcces);

                        }
                        else if (IsCompressed())//Le fichier est compressé
                        {
                            Stopwatch sw=new Stopwatch();
                            sw.Start();
                            Console.WriteLine("Niveau intermédiare compréssé");
                            MemoryStream temp_st_2 = new MemoryStream();
                            Top.Decode(temp_st, temp_st_2, -1, -1);
                            //on déccode le block
                            BlockType temp = new BlockType(top, 0, 0);
                            temp_st_2.Seek(0, SeekOrigin.Begin);
                            sw.Stop();
                            Console.WriteLine("deccodage en:" + sw.Elapsed);
                            temp.ReadHeader(temp_st_2);//on lis le header de depuis le stream qui à été décompréssé
                            return temp.RetrieveFrame(f, temp_st_2, outputSize, DirectAcces);
                        }
                        else
                        {//on est pas compressé donc on peut se contenter d'ouvrir le steam au bonne endroit

                            BlockType temp = new BlockType(top, pred, 0);

                            temp.ReadHeader(temp_st);//on lis le header
                            return temp.RetrieveFrame(f, temp_st, outputSize, DirectAcces);
                        }

                    }
                    else// cas si on est un bloque de base
                    {
                        if (IsCompressed())//Le fichier est compressé
                        {
                            MemoryStream temp_st_2 = new MemoryStream();

                            Top.Decode(temp_st, temp_st_2, -1, -1);
                            //on déccode le block
                            outputSize = -1;
                            return temp_st_2;//le block que l'on a décodé est la frame que l'on cherche
                        }
                        else
                        {//on est pas compressé donc on peut se contenter d'ouvrir le steam au bonne endroit

                            //on connait la taille du frame non encodé gràce à redBlockSize
                            long end_pos = PredictFramePosition(f + 1, temp_st);
                            int size;
                            if (end_pos > 0)
                            {
                                size = (int)(end_pos - temp_st.Position);
                            }
                            else
                            {
                                size = (int)(temp_st.Length - temp_st.Position);
                            }
                            outputSize = size;

                            if (DirectAcces)
                            {
                                Console.WriteLine("sortie direct taille:" + size);
                                
                                return temp_st;
                            }
                            else
                            {
                                byte[] t = new byte[size];

                                temp_st.Read(t, 0, size);
                                MemoryStream temp_st_2 = new MemoryStream(t, 0, size, true, true);
                                temp_st_2.Seek(0, SeekOrigin.Begin);
                               
                                Console.WriteLine("sortie taille:" + temp_st_2.Length + "pourtant=" + size);
                                return temp_st_2;
                            }
                            
   
                        }

                    }
                }
            }
            else
            {
                throw new Exception();
               return new MemoryStream();//erreur on retourne un steam vide
            }
        }


    }
    class BlockLeaf : BlockType//C'est le niveau le plus bas (ne peut pas créer d'abre
    {
        public override int Depth => 0;
        override public bool AddFrame(Stream st)
        {
            long sp = StartPosition + TotalSize;
            System.Console.WriteLine("block écris en " + sp.ToString());
            if (Top.EncodingNeeded(0))
            {
                FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé
                


                Top.Encode(st, FileSource, -1, -1);
                BlockMarker(FileSource);//marker de fin de block
                addBlockSize(sp, (FileSource.Position - sp) + 1, 1);
            }
            else
            {
                FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé
                
                st.CopyTo(FileSource);
                BlockMarker(FileSource);//marker de fin de block
                addBlockSize(sp, st.Length + 1, 1);

            }
            Console.WriteLine("taille : " + TotalSize.ToString());
            blockCount++;
            if (PredictHit(FileSource) && !Top.MaxBlockCountReach(0, blockCount))
            {
                UpdateHeader(FileSource);
                return false;//pas de problème
            }
            else
            {

                TerminateBlock(FileSource);
                if (!Top.MaxBlockCountReach(0, blockCount))
                {
                    Console.WriteLine("echec predicteur d=" + Depth + ", on fini avec Totalize="+TotalSize + "(entre "+FirstFrame+ " et "+LastFrame+ ")");
                }
                return true;
            }

        }
        override public void FinishTree(Stream source)
        {
            TerminateBlock(source);
        }
        public BlockLeaf(CoreDumpFileTop top_a, Stream st, long first_frame = 0, bool no_write = false) : base(top_a, st.Position,first_frame)
        {
            configuration |= (byte)config_flags.BASE;//mis en mode base
            FileSource = st;
            if (!no_write) WriteHeader(FileSource);
        }


    }
    class BlockTree : BlockType
    {
        protected Int32 depth = 1;
        public override int Depth => depth;
        protected BlockType Child;//chaque arbre n'as qu'un seul enfant actif les autres sont des Bloc Encodé
        public BlockTree(CoreDumpFileTop top_a, Stream st, Int32 d, long first_frame=0,bool no_write = false) : base(top_a, st.Position,first_frame)
        {
            FileSource = st;
            depth = d;
            if (!no_write)
            {

                WriteHeader(FileSource);//on prend en compte la taille du header
                
                CreateNewChild(FileSource);//on descend jusqu'en bas de l'abre
            }
            
        }
        //retourne la profondeur de l'abre restoré(=Depth)
        internal int RestoreTree()//le fichier source est toujours celui du top TODO (block file compa)
        {
            //on se place au début du block incomplet

            BlockType temp = new BlockType(Top, StartPosition+LastAddedBlockPos,0);
            temp.UpdateHeaderFromFile();
            temp.GoStartIndex(Top.Output);
            if (temp.isBaseBlock())
            {
                Child = new BlockLeaf(Top, Top.Output,0, true);
                Child.UpdateHeaderFromFile();
                return 0;
            }
            else
            {//on a encore un arbre en dessous
                BlockTree t_child = new BlockTree(Top, Top.Output, 1, 0,true);
                Child = t_child;
                Child.UpdateHeaderFromFile();
                //on recalcul la depth
                depth = t_child.RestoreTree() + 1;
                return Depth;
            }
        }

        internal virtual void CreateNewChild(Stream st)
        {
            long pos = st.Position;
            Child=Top.CreateNewBlock(depth - 1, st, LastFrame, false);
            Console.WriteLine("Création récursive d'enfant : première frame" + Child.FirstFrame + " depth=" + depth + " en "+pos);
        }
        internal  virtual void addChildBlock(BlockType b, Stream bl_st,bool force_copy=false)
        {
            if (Top.EncodingNeeded(depth) || IsCompressed())//on doit encoder ?
            {
                SetCompressed();
                b.GoStartIndex(bl_st);
                using (FileStream childCopy = new FileStream("temp", FileMode.Create))
                {
                    bl_st.CopyTo(childCopy);
                    FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé
                    childCopy.Seek(0, SeekOrigin.Begin);
                    childCopy.SetLength(b.TotalSize);

                    long start_pos = FileSource.Position;

                    Top.Encode(childCopy, FileSource, b.TotalSize, -1);//encodage de l'abre enfant
                    BlockMarker(FileSource);
                    long compress_size = (FileSource.Position - start_pos);

                    addBlockSize(start_pos, compress_size, b.FrameInBlock, b.FirstFrame);//la taille à changé puisqu'on encode 
                    Console.WriteLine("arbre (d=" + Depth + ") contenant les frames " + b.FirstFrame + " à " + b.LastFrame + " encodé à la position :" + LastAddedBlockPos.ToString() + "réduit de " + b.TotalSize + " à " + compress_size);
                }
                
            }
            else
            {
                
                if (force_copy)
                {
                    
                    b.GoStartIndex(bl_st);
                    using (FileStream childCopy = new FileStream("temp", FileMode.Create))
                    {
                        bl_st.CopyTo(childCopy);
                        childCopy.Seek(0, SeekOrigin.Begin);
                        Top.Output.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé

                        childCopy.SetLength(b.TotalSize);
                        childCopy.CopyTo(FileSource);
                        BlockMarker(FileSource);//marker de début de block  
                        addBlockSize(StartPosition + TotalSize, b.TotalSize + 1, b.FrameInBlock, b.FirstFrame);//taille est inchangé

                        Console.WriteLine("arbre (d=" + Depth + ") contenant les frames " + b.FirstFrame + " à " + b.LastFrame + "  copié à la position :" + LastAddedBlockPos.ToString() + "de taille" + childCopy.Length);
                    }
                }
                else
                {
                    //le block est déja dans le fichier
                    addBlockSize(b.StartPosition, b.TotalSize+1, b.FrameInBlock, b.FirstFrame);//taille est inchangé$
                    Top.Output.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met à la fin
                    BlockMarker(FileSource);
                    
                    
                    Console.WriteLine("arbre (d=" + Depth + ") contenant " + b.FirstFrame + " à " + b.LastFrame + "  laissé à la position :" + LastAddedBlockPos.ToString() + "de taille" +b.TotalSize);
                }

                
            }
            blockCount++;//on a un block de plus
        }


        override public bool AddFrame(Stream st)
        {
            long adding_pos = FileSource.Position;
            System.Console.WriteLine("arbe niveau " + depth.ToString() + " en " + StartPosition.ToString() + "avec " + blockCount.ToString() + " block, contenant de "+FirstFrame +" à " + LastFrame);
            if (Child.AddFrame(st))//on a un problème
            {
                
                addChildBlock(Child, FileSource, false);
                if (PredictHit(FileSource) && !Top.MaxBlockCountReach(depth, blockCount))//pas de problème on peut faire une nouvelle branche
                {
                    CreateNewChild(FileSource);
                    UpdateHeader(FileSource);

                    return false;
                }
                else//le predicteur à échouer il faut arrêter
                {
                    TerminateBlock(FileSource);//on met le Block comme terminé (update aussi le header)
                    if (!Top.MaxBlockCountReach(0, blockCount))
                    {
                        Console.WriteLine("echec predicteur d=" + Depth + ", on fini avec Totalize=" + TotalSize + "(entre " + FirstFrame + " et " + LastFrame + ")");
                    }
                    Console.WriteLine("abre terminé");
                    return true;//le problème doit remonter à l'arbre au dessus
                }
            }
            else
            {

                return false;//pas de problème
            }
        }
        override public void FinishTree(Stream source)
        {
            Child.FinishTree(source);
            addChildBlock(Child, source, false);
            TerminateBlock(source);//on met le Block comme terminé (update aussi le header)
        }


    }
    class BlockFileTree : BlockTree{
        FileStream CurrentNodeFile;
        string CurrentNodeFileName;
        public BlockFileTree(CoreDumpFileTop top_a, Stream st, Int32 d,long first_frame = 0, bool no_write = false) : base(top_a,st,d,first_frame,true)
        {
            FileSource = st;
            depth = d;
            SetExternFile();
            if (!no_write)
            {
                
                WriteHeader(FileSource);//on prend en compte la taille du header
                CreateNewChild(CurrentNodeFile);//on descend jusqu'en bas de l'abre
            }
            
        }
        public FileStream MakeNodeFileTemp()
        {
            CurrentNodeFile.Close();
            File.Delete(CurrentNodeFileName + ".2");
            File.Move(CurrentNodeFileName, CurrentNodeFileName + ".2");
            OpenFile(CurrentNodeFileName);
            return new FileStream(CurrentNodeFileName + ".2",FileMode.Open, FileAccess.Read);
        }
        override internal void CreateNewChild(Stream st)
        {
            CreateNewChildFile();
            base.CreateNewChild(CurrentNodeFile);
        }

        //TODO addChildBlockFile et remplacer add childblock
        internal void addChildBlockFile(BlockType b)//child source est toujours le fichier FileNode
        {
            if (!CurrentNodeFile.CanWrite)
            {
                CreateNewChildFile();
            }
            if (Top.EncodingNeeded(depth) || IsCompressed())//on doit encoder ?
            {
                SetCompressed();
                b.GoStartIndex(FileSource);
                using (FileStream childCopy = MakeNodeFileTemp())
                {
                    
                    FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé
                    childCopy.Seek(0, SeekOrigin.Begin);
                    //childCopy.SetLength(b.TotalSize);

                    long start_pos = FileSource.Position;

                    Top.Encode(childCopy, CurrentNodeFile, b.TotalSize, -1);//encodage de l'abre enfant dans son fichier

                    FileSource.Write(Encoding.ASCII.GetBytes(CurrentNodeFileName), 0, CurrentNodeFileName.Length);//ce n'est pas les nom des fichier qui sont encodé mais 
                    BlockMarker(FileSource);
                    long compress_size = (FileSource.Position - start_pos);

                    addBlockSize(start_pos, compress_size, b.FrameInBlock, b.FirstFrame);//la taille à changé puisqu'on encode 
                    Console.WriteLine("arbre (d=" + Depth + ") contenant les frames " + b.FirstFrame + " à " + b.LastFrame + " encodé à la position :" + LastAddedBlockPos.ToString() + "réduit de " + b.TotalSize + " à " + compress_size);
                }
                File.Delete(CurrentNodeFileName + ".2");
            }
            else
            {
                    //le block est déja dans le fichier
                    FileSource.Seek(StartPosition + TotalSize, SeekOrigin.Begin);//on se met la ou on doit ajouter le block encodé
                    long start_pos = FileSource.Position;

                    FileSource.Write(Encoding.ASCII.GetBytes(CurrentNodeFileName), 0, CurrentNodeFileName.Length);//ce n'est pas les nom des fichier qui sont encodé mais 
                    BlockMarker(FileSource);

                    long compress_size = (FileSource.Position - start_pos);

                    addBlockSize(start_pos, compress_size, b.FrameInBlock, b.FirstFrame);//la taille à changé puisqu'on encode 

                    Console.WriteLine("arbre (d=" + Depth + ") contenant " + b.FirstFrame + " à " + b.LastFrame + "  laissé à la position :" + LastAddedBlockPos.ToString() + "de taille" + b.TotalSize);
                


            }
            blockCount++;//on a un block de plus
        }

        private void OpenFile(string name)
        {
            if (CurrentNodeFile != null) CurrentNodeFile.Close();
            CurrentNodeFileName = name;
            CurrentNodeFile = new FileStream(name, FileMode.OpenOrCreate);
            Console.WriteLine("nouveau fichier crée :" + CurrentNodeFileName);
        }

        private void CreateNewChildFile()
        {
            string path = Top.SeperateFileName(depth, blockCount);
            File.Delete(path);
            OpenFile(path);
            
        }
        
        public override void FinishTree(Stream source)
        {
            Child.FinishTree(CurrentNodeFile);
            addChildBlockFile(Child);
            TerminateBlock(source);//on met le Block comme terminé (update aussi le header)
            CurrentNodeFile.Close();
        }

        public override bool AddFrame(Stream st)
        {//TODO compa with
            long adding_pos = CurrentNodeFile.Position;
            System.Console.WriteLine("arbe niveau " + depth.ToString() + " en " + StartPosition.ToString() + "avec " + blockCount.ToString() + " block, contenant de " + FirstFrame + " à " + LastFrame);
            if (Child.AddFrame(st))//on a un problème
            {
                //comportement différent il faut ajouter le fichier(c'est à dire son nomn) mais pas le contenu 
                addChildBlockFile(Child);//AddChildFile


                if (PredictHit(FileSource) && !Top.MaxBlockCountReach(depth, blockCount))//pas de problème on peut faire une nouvelle branche
                {
                    CreateNewChildFile();
                    CreateNewChild(FileSource);
                    UpdateHeader(FileSource);

                    return false;
                }
                else//le predicteur à échouer il faut arrêter
                {
                    TerminateBlock(FileSource);//on met le Block comme terminé (update aussi le header)
                    CurrentNodeFile.Close();
                    if (!Top.MaxBlockCountReach(0, blockCount))
                    {
                        Console.WriteLine("echec predicteur d=" + Depth + ", on fini avec Totalize=" + TotalSize + "(entre " + FirstFrame + " et " + LastFrame + ")");
                    }
                    Console.WriteLine("abre terminé");
                    return true;//le problème doit remonter à l'arbre au dessus
                }
            }
            else
            {

                return false;//pas de problème
            }
        }

        internal override void addChildBlock(BlockType b, Stream bl_st, bool force_copy = false)
        {
            //on copie indépendament
            b.GoStartIndex(bl_st);
            if (CurrentNodeFile==null)
            {
                CreateNewChildFile();
            }
            //le fichier utilisé est nécéssairement nouveau
            bl_st.CopyTo(CurrentNodeFile);
            addChildBlockFile(b);

        }
        ~BlockFileTree()
        {
            CurrentNodeFile.Close();
        }
    }
}
