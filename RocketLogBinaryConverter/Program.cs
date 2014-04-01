using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConvertBinaryRocketLog
{
    class Program
    {
        static void Main(string[] args)
        {
            var f = File.Open(args[0], FileMode.Open);

            byte[] buf = new byte[f.Length];
            f.Read(buf, 0, buf.Length);

            f.Close();


            using (var f2 = File.Create(args[1], 1024))
            {
                StreamWriter sw = new StreamWriter(f2);

                for (int i = 0; i < buf.Length; i += 8)
                {
                    UInt32 ts = BitConverter.ToUInt32(buf, i);
                    float press = BitConverter.ToSingle(buf, i + 4);
                    sw.WriteLine(ts + "\t" + press);
                }

                f2.Flush();
                f2.Close();
            }
        }
    }
}
