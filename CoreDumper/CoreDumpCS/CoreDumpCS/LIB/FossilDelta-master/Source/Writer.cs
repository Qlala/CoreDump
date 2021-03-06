using System;
using System.Collections.Generic;
using System.IO;

namespace Fossil
{
	public class Writer
	{
		static readonly uint[] zDigits = {
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
			'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
			'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', 'a', 'b', 'c', 'd', 'e',
			'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
			't', 'u', 'v', 'w', 'x', 'y', 'z', '~'
		};

		private List<byte> a;

		public Writer ()
		{
			this.a = new List<byte>();
		}

		public void PutChar (char c)
		{
			this.a.Add ((byte) c);
		}	

		public void PutInt (uint v)
		{
			int i, j;
			uint[] zBuf = new uint[20];

			if (v == 0) {
				this.PutChar ('0');
				return;
			}
			for (i = 0; v > 0; i++, v>>=6) {
				zBuf[i] = zDigits[v&0x3f];
			}
			for (j = i - 1; j >= 0; j--) {
				this.a.Add ((byte) zBuf [j]);
			}
		}

		public void PutArray (byte[] a, int start, int end) {
			for (var i = start; i < end; i++) this.a.Add(a[i]);
		}
	public void PutArray(Stream a, int start, int end)
	{
	  byte c;
	  a.Seek(start, SeekOrigin.Begin);
	  for (var i = start; i < end; i++)
	  {
		c =(byte) a.ReadByte();
		this.a.Add(c);
	  }
	}

	public byte[] ToArray ()
		{
			return this.a.ToArray();
		}

	}

  public class StreamWriter
  {
	static readonly uint[] zDigits = {
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
			'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
			'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', 'a', 'b', 'c', 'd', 'e',
			'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
			't', 'u', 'v', 'w', 'x', 'y', 'z', '~'
		};

	MemoryStream a;

	public MemoryStream BaseStream { get => a; }

	public StreamWriter()
	{
	  a = new MemoryStream();
	}

	public void PutChar(char c)
	{
	  a.WriteByte((byte)c);
	  //this.a.Add((byte)c);
	}

	public void PutInt(uint v)
	{
	  int i, j;
	  uint[] zBuf = new uint[20];

	  if (v == 0)
	  {
		this.PutChar('0');
		return;
	  }
	  for (i = 0; v > 0; i++, v >>= 6)
	  {
		zBuf[i] = zDigits[v & 0x3f];
	  }
	  for (j = i - 1; j >= 0; j--)
	  {
		a.WriteByte((byte)zBuf[j]);
	  }
	}

	public void PutArray(byte[] a, int start, int end)
	{
	  this.a.Write(a, start, end - start);
	}
	public void PutArray(Stream a, int start, int end)
	{
	  byte c;
	  a.Seek(start, SeekOrigin.Begin);
	  for (var i = start; i < end; i++)
	  {
		c = (byte)a.ReadByte();
		this.a.WriteByte(c);
	  }
	}

	public byte[] ToArray()
	{
	  return this.a.GetBuffer();
	}

  }


}

