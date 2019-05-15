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
    class DeltaLib
    {
        const int DELTA_WINDOW_SIZE = 1024;
        static public MemoryStream ApplyDelta(Stream delta, Stream source)
        {
            MemoryStream output = new MemoryStream();
            byte[] buff = new byte[DELTA_WINDOW_SIZE];
            while (delta.Position < delta.Length)
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
            output.Position = 0;
            return output;
        }
        static public byte[] RA_ApplyDelta(long pos, long size, Stream delta, Stream source)
        {
            byte[] output = new byte[size];
            long approx_pos = (pos / DELTA_WINDOW_SIZE) * DELTA_WINDOW_SIZE;
            long data_copied = 0;
            long curr_pos = 0;
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

            source.Position += curr_pos;
            if (curr_pos < pos)
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

            while (data_copied + DELTA_WINDOW_SIZE < size)
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
                buff.CopyTo(output, data_copied);
                data_copied += DELTA_WINDOW_SIZE;
            }
            buff = new byte[size - data_copied];
            if (delta.ReadByte() == 'C')
            {
                source.Read(buff, 0, (int)(size - data_copied));
            }
            else//P
            {
                delta.Read(buff, 0, (int)(size - data_copied));
            }
            buff.CopyTo(output, data_copied);
            return output.ToArray();
        }
    }
    class CoreDumpDeltaOpener<U> : CoreDumpOpener<U> where U : ICode
    {
        //fast deccode kept frame

        SortedDictionary<long, Stream> fastDecode_keptFrame = new SortedDictionary<long, Stream>();
        SortedDictionary<long, long> fastDecode_keptFrame_pos = new SortedDictionary<long, long>();
        public CoreDumpDeltaOpener(U d):base(d)
        {
        }

        void KeepFrame(Stream kept_frame, long f)
        {
            Console.WriteLine("Sauvegarde de la frame:" + f);
            fastDecode_keptFrame.Add(f, kept_frame);
            fastDecode_keptFrame_pos.Add(f, kept_frame.Position);
            if (fastDecode_keptFrame.Count > 3)
            {
                KeyValuePair<long, Stream> f_to_remove = fastDecode_keptFrame.First<KeyValuePair<long, Stream>>();
                fastDecode_keptFrame.Remove(f_to_remove.Key);
                fastDecode_keptFrame_pos.Remove(f_to_remove.Key);
                Console.WriteLine("remove key=" + f_to_remove.Key);
            }
        }

        Stream SearchOriginalFrame(long f, int outputsize)
        {
            if (fastDecode_keptFrame.ContainsKey(f))
            {
                Console.WriteLine("fast deccord frame:" + f);
                fastDecode_keptFrame[f].Seek(fastDecode_keptFrame_pos[f], SeekOrigin.Begin);
                return fastDecode_keptFrame[f];
            }
            else
            {
                Stream frame = DirectRetriveFrame(f, outputsize);
                frame.ReadByte();
                KeepFrame(frame, f);
                return frame;
            }
        }

        public MemoryStream ResolveDelta(Stream st, long f)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize = 0;
            Stream ref_frame = SearchOriginalFrame(f - delta_ref_f, outputsize);
            //ref_frame.ReadByte();

            return DeltaLib.ApplyDelta(st, ref_frame);


        }
        public byte[] DA_ResolveDelta(Stream st, long f, long pos, long size)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize = 0;
            Stream ref_frame = SearchOriginalFrame(f - delta_ref_f, outputsize);
            //ref_frame.ReadByte();
            //return Fossil.Delta.RA_ApplyStream(pos,size,ref_frame, st);
            return DeltaLib.RA_ApplyDelta(pos, size, st, ref_frame);

        }


        public override byte[] randomAccesFrame(long f, long pos, long size)
        {
            Console.WriteLine("random acces : frame=" + f);

            int outputsizea = 0;
            if (fastDecode_keptFrame.ContainsKey(f))
            {
                fastDecode_keptFrame[f].Seek(fastDecode_keptFrame_pos[f], SeekOrigin.Begin);
                List<byte> a = new List<byte>();
                fastDecode_keptFrame[f].Seek(pos, SeekOrigin.Current);
                for (int i = 0; i < size; i++) a.Add((byte)fastDecode_keptFrame[f].ReadByte());
                return a.ToArray();
            }
            else
            {
                Stopwatch sw = new Stopwatch();
                sw.Start();
                Stream temp = DirectRetriveFrame(f, outputsizea);
                sw.Stop();
                Console.WriteLine("temps recuperation delta:" + sw.Elapsed);
                sw.Reset();
                //temp.Seek(0, SeekOrigin.Begin);
                if (temp.ReadByte() == 0)
                {
                    KeepFrame(temp, f);
                    List<byte> a = new List<byte>();
                    temp.Seek(pos, SeekOrigin.Current);
                    for (int i = 0; i < size; i++) a.Add((byte)temp.ReadByte());
                    return a.ToArray();
                }
                else
                {
                    return DA_ResolveDelta(temp, f, pos, size);
                }
            }
        }

        public override Stream RetriveFrame(long f)
        {
            if (fastDecode_keptFrame.ContainsKey(f))
            {
                fastDecode_keptFrame[f].Seek(fastDecode_keptFrame_pos[f], SeekOrigin.Begin);
                MemoryStream copy = new MemoryStream();
                fastDecode_keptFrame[f].CopyTo(copy);
                copy.Seek(0, SeekOrigin.Begin);
                return copy;
            }
            else { 
                Stopwatch sw = new Stopwatch();
                sw.Start();
                Stream temp = base.RetriveFrame(f);
                sw.Stop();
                Console.WriteLine("temps recuperation delta:" + sw.Elapsed);
                sw.Reset();
                temp.Seek(0, SeekOrigin.Begin);
                if (temp.ReadByte() == 0)
                {
                    KeepFrame(temp, f);
                    MemoryStream temp2 = new MemoryStream();
                    temp.CopyTo(temp2);
                    temp2.Seek(0, SeekOrigin.Begin);
                    return temp2;
                }
                else
                {
                    return ResolveDelta(temp, f);
                }

            }
        }
    }




    class CoreDumpDeltaWriter<T, U> : CoreDumpWriter<T, U> where T : ICode where U : ICode
    {
        static long DELTA_TH = 50000;
        MemoryStream current_reference = new MemoryStream();

        long reference_id = -1;
        long last_added_frame = -1;

        //fast deccode kept frame
        Dictionary<long, Stream> fastDecode_keptFrame = new Dictionary<long, Stream>();
        Dictionary<long, long> fastDecode_keptFrame_pos = new Dictionary<long, long>();

        //TODO=> remplacer avec la version actuelle
        public Stream ComputeDelta(Stream st, long f)
        {
            Stopwatch sw = new Stopwatch();
            st.Seek(0, SeekOrigin.Begin);
            sw.Start();
            byte[] in_buff = new byte[st.Length];

            st.Read(in_buff, 0, (int)st.Length);
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
            out_st.Write(BitConverter.GetBytes((int)(f - reference_id)), 0, 4);
            out_st.Write(out_buff, 0, out_buff.Length);
            out_st.Seek(0, SeekOrigin.Begin);
            sw.Stop();
            Console.WriteLine("temps copie sortie:" + sw.Elapsed);
            return out_st;
        }
        
        Stream MakeNewReference(Stream st, long f)
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
        void KeepFrame(Stream kept_frame, long f)
        {
            Console.WriteLine("Sauvegarde de la frame:" + f);
            fastDecode_keptFrame.Add(f, kept_frame);
            fastDecode_keptFrame_pos.Add(f, kept_frame.Position);
            if (fastDecode_keptFrame.Count > 3)
            {
                KeyValuePair<long, Stream> f_to_remove = fastDecode_keptFrame.First<KeyValuePair<long, Stream>>();
                fastDecode_keptFrame.Remove(f_to_remove.Key);
                fastDecode_keptFrame_pos.Remove(f_to_remove.Key);
                Console.WriteLine("remove key=" + f_to_remove.Key);
            }
        }

        Stream SearchOriginalFrame(long f, int outputsize)
        {
            if (fastDecode_keptFrame.ContainsKey(f))
            {
                Console.WriteLine("fast deccord frame:" + f);
                fastDecode_keptFrame[f].Seek(fastDecode_keptFrame_pos[f], SeekOrigin.Begin);
                return fastDecode_keptFrame[f];
            }
            else
            {
                Stream frame = DirectRetriveFrame(f, outputsize);
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
                Stream delta = ComputeDelta(frame, last_added_frame);
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



       
        public MemoryStream ResolveDelta(Stream st, long f)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize = 0;
            Stream ref_frame = SearchOriginalFrame(f - delta_ref_f, outputsize);
            ref_frame.ReadByte();
            
            return DeltaLib.ApplyDelta(st, ref_frame);


        }
        public byte[] DA_ResolveDelta(Stream st, long f, long pos, long size)
        {
            byte[] bytes = new byte[4];
            st.Read(bytes, 0, 4);


            int delta_ref_f = BitConverter.ToInt32(bytes, 0);
            int outputsize = 0;
            Stream ref_frame = SearchOriginalFrame(f - delta_ref_f, outputsize);
            ref_frame.ReadByte();
            //return Fossil.Delta.RA_ApplyStream(pos,size,ref_frame, st);
            return DeltaLib.RA_ApplyDelta(pos, size, st, ref_frame);

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
                return DA_ResolveDelta(temp, f, pos, size);
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
