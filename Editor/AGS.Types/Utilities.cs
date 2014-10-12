using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;

namespace AGS.Types
{
    public static class Utilities
    {
        public static string RemoveInvalidCharactersFromScriptName(string name)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < name.Length; i++)
            {
                if ((IsEnglishLetterOrDigit(name[i])) || (name[i] == '_'))
                {
                    sb.Append(name[i]);
                }
                else
                {
                    sb.Append('_');
                }
            }
            return sb.ToString();
        }

        private static bool IsEnglishLetterOrDigit(char theChar)
        {
            if ((theChar >= 'A') && (theChar <= 'Z')) return true;
            if ((theChar >= 'a') && (theChar <= 'z')) return true;
            if ((theChar >= '0') && (theChar <= '9')) return true;
            return false;
        }

        public static string ValidateScriptName(string name, int truncateToLength)
        {
            for (int i = 0; i < name.Length; i++)
            {
                if ((i == 0) && (Char.IsDigit(name[i])))
                {
                    throw new InvalidDataException("Script name cannot start with number: " + name);
                }
                if ((!IsEnglishLetterOrDigit(name[i])) && (name[i] != '_'))
                {
                    throw new InvalidDataException("Invalid script name; name must only include letters, numbers and underscores: " + name);
                }
            }
            if (name.Length > truncateToLength)
            {
                return name.Substring(0, truncateToLength);
            }
            return name;
        }

        public static string ValidateScriptName(string name)
        {
            return ValidateScriptName(name, 99999);
        }

        /// <summary>
        /// Returns the directory from which the editor is running.
        /// RequiredAGSVersion: 3.3.1.1165
        /// </summary>
        public static string EditorDirectory
        {
            get
            {
                return Path.GetDirectoryName(Process.GetCurrentProcess().MainModule.FileName);
            }
        }

        public static Size UserStringToResolution(string s)
        {
            String[] parts = s.Split('x');
            return new Size(Int32.Parse(parts[0]), Int32.Parse(parts[1]));
        }

        public static string ResolutionToUserString(Size size)
        {
            return String.Format("{0} x {1}", size.Width, size.Height);
        }
    }
}
