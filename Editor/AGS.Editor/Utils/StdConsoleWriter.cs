using System;
using System.IO;

namespace AGS.Editor
{
    // based on the code from the answer here https://stackoverflow.com/a/14199852

    public sealed class StdConsoleWriter
    {
        private StdConsoleWriter()
        {
            AttachConsole(ATTACH_PARENT_PROCESS);
        }
        private void WriteOutLine(string line)
        {
            Console.WriteLine("AGS: " + line);
        }

        private static bool _on;
        private static StdConsoleWriter _instance;
        private static StdConsoleWriter GetInstance()
        {
            if (_instance == null)
            {
                _instance = new StdConsoleWriter();
            }
            return _instance;
        }

        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        private static extern bool AttachConsole(int dwProcessId);

        private const int ATTACH_PARENT_PROCESS = -1;

        static public void Enable()
        {
            _on = true;
        }

        static public bool IsEnabled
        {
            get { return _on; }
        }

        static public void WriteLine(string line)
        {
            if (_on)
            {
                GetInstance().WriteOutLine(line);
            }
        }

    }
}