using System.Runtime.InteropServices;
using System;

namespace Scintilla
{
    public enum BeepType
    {
        Default = -1,
        Ok = 0x00000000,
        Error = 0x00000010,
        Question = 0x00000020,
        Warning = 0x00000030,
        Information = 0x00000040,
    }

    public class WinAPI
    {
        public const int WM_NOTIFY = 0x004e;
        public const int WM_SETCURSOR = 0x0020;

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool MessageBeep(BeepType type);

        [DllImport("kernel32.dll")]
        public static extern bool Beep(int frequency, int time);

        [DllImport("kernel32")]
        public extern static IntPtr LoadLibrary(string lpLibFileName);

        [DllImport("user32")]
        public static extern IntPtr CreateWindowEx(
            uint dwExStyle, string lpClassName, string lpWindowName,
            uint dwStyle, int x, int y, int width, int height, IntPtr hWndParent,
            int hMenu, IntPtr hInstance, string lpParam);

        [DllImport("kernel32", EntryPoint = "SendMessage")]
        public static extern int SendMessageStr(
            IntPtr hWnd,
            int message,
            int data,
            string s);

        [DllImport("user32.dll")]
        public static extern IntPtr SetFocus(IntPtr hwnd);
    }
}

