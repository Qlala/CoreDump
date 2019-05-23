using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

//interface dédié à alstom
using MyCoders;
using CoreDumper;

namespace Dumper
{
    class dump_reader: IDisposable
    {
        CoreDumpDeltaOpener<Deflate_Decoder> opener;
        public dump_reader(string filename)
        {
            opener = new CoreDumpDeltaOpener<Deflate_Decoder>(new Deflate_Decoder());
            opener.readOnlyOpen(filename);
        }
        public long FirstCycleRank()
        {
            return opener.RetrieveFirstFrame();
        }
        public long LastCycleRank()
        {
            return opener.RetrieveFrameCount();
        }
        public Stream GetCycle(long cycleRank)
        {
            return opener.RetriveFrame(cycleRank);
        }
        public byte[] GetBytesAtCycleAtAddress(long cycleRank, long address, long byteCount)
        {
            return opener.randomAccesFrame(cycleRank, address, byteCount);
        }

        void IDisposable.Dispose()
        {
            opener.Dispose();
        }


    }
}
