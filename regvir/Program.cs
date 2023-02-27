using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace regvir
{
    internal class Program
    {
        private void Scan(Regex regex, string l)
        {
            MatchCollection matches = regex.Matches(l);
            /*foreach (Match match in matches)*/
            Console.WriteLine(matches.Count);
        }

        static void Main(string[] args)
        {
            StreamReader sr = new StreamReader("C:\\Users\\zhaho\\OneDrive\\Рабочий стол\\Колледж\\Разработка программных модулей систем\\str.txt");
            Regex regex = new Regex(@"([\d]{6})");
            string l = sr.ReadLine();
            Program S = new Program();
            S.Scan(regex, l);
            Console.ReadLine();
        }

    }
}
