using System;
using System.IO;

namespace AGS.Editor
{
    // based on the code from the answer here https://stackoverflow.com/a/14199852

    public sealed class StdConsoleWriter
    {
        private readonly bool _attached = false;
        private bool _firstLine = true;

        private StdConsoleWriter()
        {
            // for some reason Console.IsOutputRedirect is not working for me in the old cmd.exe
            // we only want to attach if output is not being redirected
            var stdout = Console.OpenStandardOutput();
            if (stdout == Stream.Null)
            {
                _attached = AttachConsole(ATTACH_PARENT_PROCESS);
            }
        }

        private void WriteOutLine(string line)
        {
            const string prefix = "AGS: ";
            string msg = prefix + line;

            if (_attached && _firstLine)
            {
                // AGSEditor is a Windows Application and not a Console Application
                // so it will release the console before we attach again and print an unused prompt line (in cmd.exe)
                // the line below erases this line and prints the first message on top of it
                // this is only necessary once, in the first line printed, and only if not attached to a console
                ClearPrint(msg);
            }
            else 
            { 
                Console.WriteLine(msg);
            }
            _firstLine = false;
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

        private static void ClearPrint(string msg)
        {
            int count = Math.Max(Console.BufferWidth - msg.Length, 0);
            Console.WriteLine($"\r{msg}{new String(' ', count)}");
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