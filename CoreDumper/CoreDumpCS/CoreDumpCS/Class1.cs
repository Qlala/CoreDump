using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace ConsoleApp1
{
    class MaxEnthropyFrame
    {
        int size = 0;
        MemoryStream st;
        public MaxEnthropyFrame(int s)
        {
            size = s;
            st = new MemoryStream(size);
        }
        public Stream generate()
        {
            st.SetLength(size);
            Random rand = new Random();
            
            rand.NextBytes(st.GetBuffer());
            return st;
        }
        public void WriteFile(string path)
        {
            FileStream fst = new FileStream(path, FileMode.Create);
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(fst);
            fst.Dispose();
        }
    }
    class ProbaFrame
    {
        int size = 0;
        MemoryStream st;
        double proba=0;
        public ProbaFrame(int s,double proba_max_enth=0.8)
        {
            size = s;
            proba = proba_max_enth;
            st = new MemoryStream(size);
        }
        public Stream generate()
        {
            st.SetLength(size);
            Random rand = new Random();
            byte temp = (byte)rand.Next();
            for(int i = 0; i < size; i++)
            {
                if (rand.NextDouble() < proba)
                {
                    temp = (byte)rand.Next();
                    st.WriteByte(temp);
                }
                else
                {
                    st.WriteByte(temp);
                }
            }

            return st;
        }
        public void WriteFile(string path)
        {
            FileStream fst = new FileStream(path, FileMode.Create);
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(fst);
            fst.Dispose();
        }
    }
    class ProbaFrame_Delta
    {
        int size = 0;
        MemoryStream st;
        double proba = 0;
        double proba_delta;
        double proba_change;
        public Stream Frame { get { st.Position = 0; return st; } }
        public ProbaFrame_Delta(int s, double proba_max_enth = 0.8,double proba_change_a=0.01,double proba_delta_a=0.1)
        {
            size = s;
            proba = proba_max_enth;
            proba_change = proba_change_a;
            st = new MemoryStream(size);
            proba_delta = proba_delta_a;
        }
        public void delta_gene()
        {
            Random rand = new Random();
            byte temp = (byte)rand.Next();
            int nb=0;
            if (rand.NextDouble() < proba_change)
            {
                Console.WriteLine("altération des donnés");
                for (int i = 0; i < size; i++)
                {
                    if (rand.NextDouble() < proba_delta)
                    {
                        nb++;
                        temp = (byte)rand.Next();
                        st.WriteByte(temp);
                    }
                    else
                    {
                        st.Position += 1;
                    }
                }
                Console.WriteLine("Altération fini , nb=" + nb);
            }
        }
        public Stream generate()
        {
            st.SetLength(size);
            Random rand = new Random();
            byte temp = (byte)rand.Next();
            for (int i = 0; i < size; i++)
            {
                if (rand.NextDouble() < proba)
                {
                    temp = (byte)rand.Next();
                    st.WriteByte(temp);
                }
                else
                {
                    st.WriteByte(temp);
                }
            }

            return st;
        }
        public void WriteFile(string path)
        {
            FileStream fst = new FileStream(path, FileMode.Create);
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(fst);
            fst.Dispose();
        }
    }
    class RealFrame
    {
        /*
        //Loi des bit Utile => tiré le nombre de bit puis tiré un nombre de n(bit) : X ->(Uniforme([0,2^n[),ou n->Uniforme([0,32[))
        // |10M(W32) || 15M(W32)||
        // |10M(W32)| =>|5M(W32| => 250k(W32) tiré au début à chaque cycle 1KW32 parmi eux change selon la lois de bit utiles fixe;
        //+ |4M(32)| +-(20%) 500 MO(proba a précisé)  -> 20K(W32)/cycle change selon la loi des bit utiles.

       |15M(W32)|=> 25K(W32) change à cycle : tiré selon la loi des bit utiles.



        */
        int Size =100000000;
        MemoryStream st;
        public RealFrame()
        {
            st = new MemoryStream(Size);
            st.SetLength(Size);
        }
        public Stream generate()
        {

            return st;
        }
        public void WriteFile(string path)
        {
            FileStream fst = new FileStream(path, FileMode.Create);
            st.Seek(0, SeekOrigin.Begin);
            st.CopyTo(fst);
            fst.Dispose();
        }
    }

}
