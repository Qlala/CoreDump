using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using SevenZip;
using SevenZip.Compression;
using System.IO.Compression;
using System.IO;
using CoreDumper;
using ICSharpCode.SharpZipLib.Zip.Compression;
namespace MyCoders
{
    class LZMA_Encoder : SevenZip.Compression.LZMA.Encoder, ICode, IAddFrame
    {
        public LZMA_Encoder() : base()
        {
            ;
        }

        public void AddFrame(Stream st)
        {
            ;//rien
        }
    }
    class Deflate_Encoder : ICode, IAddFrame
    {
        System.IO.Compression.DeflateStream coder;


        public Deflate_Encoder()
        {
            
        }
        public void AddFrame(Stream st)
        {
            ;//rien
        }

        public void Code(Stream inStream, Stream outStream, long inSize, long outSize, ICodeProgress progress)
        {


            using (System.IO.Compression.DeflateStream DStream = new System.IO.Compression.DeflateStream(outStream, System.IO.Compression.CompressionLevel.Fastest, true))
            {
                if(inSize>0)
                    inStream.CopyTo(DStream,(int)inSize);
                else
                    inStream.CopyTo(DStream);
            }
             
        }
    }
    class Deflate_Decoder : ICode
    {
        System.IO.Compression.DeflateStream coder;


        public Deflate_Decoder()
        {

        }
        public void AddFrame(Stream st)
        {
            ;//rien
        }

        public void Code(Stream inStream, Stream outStream, long inSize, long outSize, ICodeProgress progress)
        {
            //outStream.ReadByte();
            if (outSize > 0)
            {
                outStream.SetLength(outSize);
            }
            using (ICSharpCode.SharpZipLib.Zip.Compression.Streams.InflaterInputStream DStream = new ICSharpCode.SharpZipLib.Zip.Compression.Streams.InflaterInputStream(inStream))
            {
                DStream.CopyTo(outStream);// incompatibilité avec zlib
                
            }
            /*
            System.IO.Compression.GZipStream DStream = new System.IO.Compression.GZipStream(inStream, System.IO.Compression.CompressionMode.Decompress,true);
            outStream=DStream;
            */
        }
    }

    class LZMA_Decoder : SevenZip.Compression.LZMA.Decoder, ICode, IAddFrame
    {
        public void AddFrame(Stream st)
        {
            ;//rien
        }
    }

}
