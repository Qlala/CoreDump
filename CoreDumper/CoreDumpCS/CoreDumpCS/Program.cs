using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using CoreDumper;
using System.Diagnostics;
using MyCoders;
using SevenZip;


using System.Windows.Forms;

namespace ConsoleApp1
{
    class Program
    {

        [STAThread]
        static void Main(string[] args)
        {
            exemple_UI();
        }


        static void exemple_UI()
        {

            // generate_delta_file("test.GZ.dp", 100, 100000000);
            //System.IO.Directory.SetCurrentDirectory("../../../../CoreDumpWriter/CoreDumpWriter/CoreDumpWriter");
            using (OpenFileDialog openFileDialog = new OpenFileDialog())
            {
                openFileDialog.InitialDirectory = System.IO.Directory.GetCurrentDirectory();

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    
                    System.IO.Directory.SetCurrentDirectory(Path.GetDirectoryName(openFileDialog.FileName));
                    Application.Run(new cdTestUIcs(openFileDialog.FileName));
                }
            }
        }

        static void rand_test(int ntry,int n_frame,Stream frame,CoreDumpFileTop test)
        {
            
            Random rand = new Random();
            int i;
            for(i = 0; i < ntry; i++)
            {
                
                if(!test_retrieve(rand.Next(0, n_frame), frame, test)) break;

                
                
            }
            if (i != ntry)
            {
                Console.WriteLine("echec");

            }
            else
            {
                Console.WriteLine("sucess");
            }

        }

        static bool test_retrieve(long nb, Stream frame, CoreDumpFileTop test)
        {
            //try
            //{
                Console.WriteLine("test f=" + nb);
                ;
                Stopwatch sw = new Stopwatch();

                sw.Start();
                Stream a = test.RetriveFrame(nb);

                sw.Stop();
                Console.WriteLine("temps écoulé =" + sw.Elapsed);
                //File.Delete("../../res.temp");
                using (MemoryStream res = new MemoryStream())
                {
                    a.Seek(0, SeekOrigin.Begin);
                    a.CopyTo(res);
                    res.Seek(0, SeekOrigin.Begin);
                    byte b, b2;
                    frame.Seek(0, SeekOrigin.Begin);
                    do
                    {
                        b = (byte)res.ReadByte();
                        b2 = (byte)frame.ReadByte();
                    } while (b == b2 && frame.Position != frame.Length);
                    if (b != b2)
                    {
                        Console.WriteLine("echec de " + nb + ": " + frame.Position + "/" + frame.Length);
                        throw new Exception();
                    }
                    else Console.WriteLine("réussite de f=" + nb);

                    Console.WriteLine("fini test");
                    res.Close();
                }
                
                return true;
            /*}catch(Exception e)
            {
                Console.WriteLine("echec avec exception de f=" + nb);
                throw new Exception();
                return false;
           }*/
        }



        static void Main2(string[] args)
        {

            System.Console.Out.WriteLine("test");

            string path = "test.txt";
            if (File.Exists(path))
            {

                using (FileStream fs = new FileStream(path, FileMode.Open, FileAccess.Read))
                {

                    List<char> buff = new List<char>(1024);
                    MemoryStream stream = new MemoryStream(1024);
                    fs.CopyTo(stream);
                    /*
                    //stream.Write(buffer, 0, 1024);

                    //sr.ReadBlock(buff.ToArray(), 0, 1024);
                    CoderPropID[] propIDs =
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
                    Int32 dictionary = 1 << 23;

                    Int32 posStateBits = 2;
                    Int32 litContextBits = 3; // for normal files
                                              // UInt32 litContextBits = 0; // for 32-bit data
                    Int32 litPosBits = 0;
                    // UInt32 litPosBits = 2; // for 32-bit data
                    Int32 algorithm = 2;
                    Int32 numFastBytes = 128;
                    string mf = "bt4";//match finder
                    bool eos = false;

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
                    //out stream
                    string outputName = "test.7-zip";
                    FileStream outStream = new FileStream(outputName, FileMode.Create, FileAccess.Write);

                    SevenZip.Compression.LZMA.Encoder encoder = new SevenZip.Compression.LZMA.Encoder();
                    encoder.SetCoderProperties(propIDs, properties);
                    encoder.WriteCoderProperties(outStream);
                    Int64 fileSize;
                    fileSize = fs.Length;
                    for (int i = 0; i < 8; i++)
                        outStream.WriteByte((Byte)(fileSize >> (8 * i)));
                    encoder.Code(fs, outStream, fs.Length, -1, null);
                    encoder.Code(fs, outStream, fs.Length, -1, null);
                    *//*
                    File.Delete("test.lzma2");
                    using (CoreDumper<LZMA_Encoder> dumper = new CoreDumper<LZMA_Encoder>("test.lzma2", 0))
                    {

                        dumper.addFrame(stream);
                        dumper.addFrame(stream);
                        dumper.addFrame(stream);
                    }
                    File.Delete("test_o.txt");
                    FileStream outfile = new FileStream("test_o.txt", FileMode.Create, FileAccess.Write);
                    using (FramExtracter<LZMA_Decoder> extracter = new FramExtracter<LZMA_Decoder>("test.lzma2"))
                    {
                        extracter.RetrieveFrame(1).CopyTo(outfile);
                    }
                    

                    while (!Console.KeyAvailable) ;
                    */
                }



            }
            else
            {

                Console.WriteLine("no file : Pres any key");
                while (!Console.KeyAvailable) ;
            }
        }


        /*
         * 
  -a{N}:  set compression mode 0 = fast, 1 = normal
          default: 1 (normal)

  d{N}:   Sets Dictionary size - [0, 30], default: 23 (8MB)
          The maximum value for dictionary size is 1 GB = 2^30 bytes.
          Dictionary size is calculated as DictionarySize = 2^N bytes. 
          For decompressing file compressed by LZMA method with dictionary 
          size D = 2^N you need about D bytes of memory (RAM).

  -fb{N}: set number of fast bytes - [5, 273], default: 128
          Usually big number gives a little bit better compression ratio 
          and slower compression process.

  -lc{N}: set number of literal context bits - [0, 8], default: 3
          Sometimes lc=4 gives gain for big files.

  -lp{N}: set number of literal pos bits - [0, 4], default: 0
          lp switch is intended for periodical data when period is 
          equal 2^N. For example, for 32-bit (4 bytes) 
          periodical data you can use lp=2. Often it's better to set lc0, 
          if you change lp switch.

  -pb{N}: set number of pos bits - [0, 4], default: 2
          pb switch is intended for periodical data 
          when period is equal 2^N.

  -mf{MF_ID}: set Match Finder. Default: bt4. 
              Algorithms from hc* group doesn't provide good compression 
              ratio, but they often works pretty fast in combination with 
              fast mode (-a0).

              Memory requirements depend from dictionary size 
              (parameter "d" in table below). 

               MF_ID     Memory                   Description

                bt2    d *  9.5 + 4MB  Binary Tree with 2 bytes hashing.
                bt3    d * 11.5 + 4MB  Binary Tree with 3 bytes hashing.
                bt4    d * 11.5 + 4MB  Binary Tree with 4 bytes hashing.
                hc4    d *  7.5 + 4MB  Hash Chain with 4 bytes hashing.

  -eos:   write End Of Stream marker. By default LZMA doesn't write 
          eos marker, since LZMA decoder knows uncompressed size 
          stored in .lzma file header.

  -si:    Read data from stdin (it will write End Of Stream marker).
  -so:    Write data to stdout
         * 
         * 
         * 
         * */

        static void generate_file(string file_name,int n_frame,int frame_size)
        {
            ProbaFrame gen = new ProbaFrame(frame_size, 0.1);
            Stream test_frame = gen.generate();
            using (CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder> test = new CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder>())
            {

                test.Coder = new Deflate_Encoder();

                CoderPropID[] propIDs =
    {
                    CoderPropID.DictionarySize,
                    CoderPropID.PosStateBits,
                    CoderPropID.LitContextBits,
                    CoderPropID.LitPosBits,
                    CoderPropID.Algorithm,
                    CoderPropID.NumFastBytes,
                    CoderPropID.MatchFinder,
                    CoderPropID.EndMarker,
                };
                Int32 dictionary = 1 << 23;

                Int32 posStateBits = 2;
                Int32 litContextBits = 0; // for normal files
                                          // UInt32 litContextBits = 0; // for 32-bit data
                Int32 litPosBits = 2;
                // UInt32 litPosBits = 2; // for 32-bit data
                Int32 algorithm = 1;
                Int32 numFastBytes = 128;
                string mf = "bt4";//match finder
                bool eos = true;

                object[] properties =
                {
                    (Int32)(dictionary),
                    (Int32)(posStateBits),
                    (Int32)(litContextBits),
                    (Int32)(litPosBits),
                    (Int32)(algorithm),
                    (Int32)(numFastBytes),
                    mf,
                    eos,

                };
                //test.Coder.SetCoderProperties(propIDs, properties);
                File.Delete(file_name);
                test.OpenNewFile(file_name);

                Console.WriteLine("frame de taille : " + test_frame.Length);
                Stopwatch sw = new Stopwatch();
                for (int i = 0; i < n_frame; i++)
                {
                    Console.WriteLine("frame " + i.ToString());
                    test_frame.Seek(0, SeekOrigin.Begin);
                    sw.Start();
                    test.AddFrame(test_frame);
                    sw.Stop();
                    Console.WriteLine("frame " + i.ToString() + " fait en " + sw.Elapsed);
                    sw.Reset();
                }
                System.Console.WriteLine("fini");
                test.FinishTree();

                //test.Coder.WriteCoderProperties(temp);

            }
        }


        static void generate_delta_file(string file_name, int n_frame, int frame_size)
        {
            ProbaFrame_Delta gen = new ProbaFrame_Delta(frame_size, 0.1,0.1,0.00001);
            Stream test_frame = gen.generate();
            using (CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder> test = new CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder>())
            {

                test.Coder = new Deflate_Encoder();

                CoderPropID[] propIDs =
    {
                    CoderPropID.DictionarySize,
                    CoderPropID.PosStateBits,
                    CoderPropID.LitContextBits,
                    CoderPropID.LitPosBits,
                    CoderPropID.Algorithm,
                    CoderPropID.NumFastBytes,
                    CoderPropID.MatchFinder,
                    CoderPropID.EndMarker,
                };
                Int32 dictionary = 1 << 23;

                Int32 posStateBits = 2;
                Int32 litContextBits = 0; // for normal files
                                          // UInt32 litContextBits = 0; // for 32-bit data
                Int32 litPosBits = 2;
                // UInt32 litPosBits = 2; // for 32-bit data
                Int32 algorithm = 1;
                Int32 numFastBytes = 128;
                string mf = "bt4";//match finder
                bool eos = true;

                object[] properties =
                {
                    (Int32)(dictionary),
                    (Int32)(posStateBits),
                    (Int32)(litContextBits),
                    (Int32)(litPosBits),
                    (Int32)(algorithm),
                    (Int32)(numFastBytes),
                    mf,
                    eos,

                };
                //test.Coder.SetCoderProperties(propIDs, properties);
                File.Delete(file_name);
                test.OpenNewFile(file_name);

                Console.WriteLine("frame de taille : " + test_frame.Length);
                Stopwatch sw = new Stopwatch();
                for (int i = 0; i < n_frame; i++)
                {
                    Console.WriteLine("frame " + i.ToString());
                    //test_frame.Seek(0, SeekOrigin.Begin);
                    gen.delta_gene();
                    sw.Start();
                    test.AddFrame(gen.Frame);
                    sw.Stop();
                    Console.WriteLine("frame " + i.ToString() + " fait en " + sw.Elapsed);
                    sw.Reset();
                }
                System.Console.WriteLine("fini");
                test.FinishTree();

                //test.Coder.WriteCoderProperties(temp);

            }
        }


        static int n_frame=300;
        static void test1(string[] args)
        {
            ProbaFrame gen = new ProbaFrame(100000000,0.1);
            Stream test_frame = gen.generate();
            gen.WriteFile("../../test_frame3.txt");
            //Stream test_frame = new FileStream("../../test_frame2.txt", FileMode.Open, FileAccess.Read);

            MemoryStream temp = new MemoryStream(5);
            using (CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder> test = new CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder>())
            {
                
                test.Coder = new Deflate_Encoder();

                CoderPropID[] propIDs =
    {
                    CoderPropID.DictionarySize,
                    CoderPropID.PosStateBits,
                    CoderPropID.LitContextBits,
                    CoderPropID.LitPosBits,
                    CoderPropID.Algorithm,
                    CoderPropID.NumFastBytes,
                    CoderPropID.MatchFinder,
                    CoderPropID.EndMarker,
                };
                Int32 dictionary = 1 << 23;

                Int32 posStateBits = 2;
                Int32 litContextBits = 0; // for normal files
                                          // UInt32 litContextBits = 0; // for 32-bit data
                Int32 litPosBits = 2;
                // UInt32 litPosBits = 2; // for 32-bit data
                Int32 algorithm = 1;
                Int32 numFastBytes = 128;
                string mf = "bt4";//match finder
                bool eos = true;

                object[] properties =
                {
                    (Int32)(dictionary),
                    (Int32)(posStateBits),
                    (Int32)(litContextBits),
                    (Int32)(litPosBits),
                    (Int32)(algorithm),
                    (Int32)(numFastBytes),
                    mf,
                    eos,

                };
                //test.Coder.SetCoderProperties(propIDs, properties);
                File.Delete("../../test_3.LZMA");
                test.OpenNewFile("../../test_3.LZMA");

                Console.WriteLine("frame de taille : " + test_frame.Length);
                Stopwatch sw = new Stopwatch();
                for (int i = 0; i < n_frame; i++)
                {
                    Console.WriteLine("frame " + i.ToString());
                    test_frame.Seek(0, SeekOrigin.Begin);
                    sw.Start();
                    test.AddFrame(test_frame);
                    sw.Stop();
                    Console.WriteLine("frame " + i.ToString()+" fait en "+sw.Elapsed);
                    sw.Reset();
                }
                System.Console.WriteLine("fini");
                test.FinishTree();

                //test.Coder.WriteCoderProperties(temp);

            }
            CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder> test2 = new CoreDumpDeltaWriter<Deflate_Encoder, Deflate_Decoder>();
            test2.readOnlyOpen("../../test_3.LZMA");
            test2.Decoder = new Deflate_Decoder();
            //test2.Decoder.SetDecoderProperties(temp.ToArray());
            test_retrieve(0, test_frame, test2);
            test_retrieve(1, test_frame, test2);
            test_retrieve(53, test_frame, test2);
            test_retrieve(260, test_frame, test2);
            test_retrieve(136, test_frame, test2);
            test_retrieve(n_frame-1, test_frame, test2);
            rand_test(100, n_frame, test_frame, test2);
            while (!Console.KeyAvailable)
                ;
        }
    }
}
