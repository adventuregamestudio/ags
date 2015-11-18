using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace AGS.Editor.Utils
{
    internal class AlternateStreams
    {
        public const uint URLZONE_PREDEFINED_MIN = 0;
        public const uint URLZONE_LOCAL_MACHINE = 0;
        public const uint URLZONE_INTRANET = 1;
        public const uint URLZONE_TRUSTED = 2;
        public const uint URLZONE_INTERNET = 3;
        public const uint URLZONE_UNTRUSTED = 4;
        public const uint URLZONE_PREDEFINED_MAX = 999;
        public const uint URLZONE_USER_MIN = 1000;
        public const uint URLZONE_USER_MAX = 10000;

        private const uint FILE_ATTRIBUTE_NORMAL = 0x80;
        private const uint GENERIC_READ = 0x80000000;
        private const uint FILE_SHARE_READ = 0x00000001;
        private const uint OPEN_EXISTING = 3;

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern SafeFileHandle CreateFile(string lpFileName, uint dwDesiredAccess,
            uint dwShareMode, IntPtr lpSecurityAttributes, uint dwCreationDisposition,
            uint dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool DeleteFile(string name);

        private static SafeFileHandle GetSafeFileHandle(string path, string streamName)
        {
            string streamPath = path + ":" + streamName;

            SafeFileHandle handle = CreateFile(streamPath, GENERIC_READ,
                FILE_SHARE_READ, IntPtr.Zero, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, IntPtr.Zero);
                
            return handle;
        }

        private static string[] ReadTextFromAlternateStream(string path, string streamName)
        {
            List<string> alternateStreamText = new List<string>();

            using (SafeFileHandle handle = GetSafeFileHandle(path, streamName))
            {
                if (handle.IsInvalid)
                {
                    return alternateStreamText.ToArray();
                }
                using (FileStream fileStream = new FileStream(handle, FileAccess.Read))
                {
                    using (StreamReader reader = new StreamReader(fileStream))
                    {
                        string line;
                        while ((line = reader.ReadLine()) != null)
                        {
                            alternateStreamText.Add(line);
                        }

                    }
                }
            }
            
            return alternateStreamText.ToArray();
        }

        public static uint GetZoneIdentifier(string path)
        {
            string[] text = ReadTextFromAlternateStream(path, "Zone.Identifier");
            string[] keyValue;

            try
            {
                keyValue = text[1].Split('=');
            }
            catch
            {
                return URLZONE_LOCAL_MACHINE;
            }

            if (keyValue[0].ToLower() == "zoneid" && keyValue.Length == 2)
            {
                uint zoneNumber;
                if (UInt32.TryParse(keyValue[1], out zoneNumber))
                {
                    return zoneNumber;
                }
            }
            
            return URLZONE_LOCAL_MACHINE;
        }

        public static bool DeleteZoneIdentifier(string path)
        {
            return DeleteAlternateStream(path, "Zone.Identifier");
        }

        private static bool DeleteAlternateStream(string path, string streamName)
        {
            return DeleteFile(path + ":" + streamName);
        }
    }
}
