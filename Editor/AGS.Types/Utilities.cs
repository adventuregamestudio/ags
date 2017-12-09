using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Text;
using System.Reflection;

namespace AGS.Types
{
    public static class Utilities
    {
        public static T GetDefaultValue<T>(Type type, string propertyName, T defaultValue)
        {
            PropertyInfo property = type.GetProperty(propertyName);
            DefaultValueAttribute[] defaults = (DefaultValueAttribute[])property.GetCustomAttributes(typeof(DefaultValueAttribute), false);
            return (defaults.Length > 0) ? (T)defaults[0].Value : defaultValue;
        }

        /// <summary>
        /// Finds second maximal value in enumeration Type.
        /// </summary>
        public static T GetSecondMaxEnumValue<T>()
        {
            Array arr = Enum.GetValues(typeof(T));
            int max = int.MinValue;
            int second = int.MinValue;
            foreach (int i in arr)
            {
                if (i > max)
                {
                    second = max;
                    max = i;
                }
                else if (i > second)
                {
                    second = i;
                }
            }
            return (T)Enum.ToObject(typeof(T), second);
        }

        public static ScriptAPIVersion GetActualAPI(ScriptAPIVersion v)
        {
            if (v == ScriptAPIVersion.Highest)
                return Utilities.GetSecondMaxEnumValue<ScriptAPIVersion>();
            return v;
        }

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

        public static Size UserStringToResolution(string s)
        {
            String[] parts = s.Split('x');
            return new Size(Int32.Parse(parts[0]), Int32.Parse(parts[1]));
        }

        public static string ResolutionToUserString(Size size)
        {
            return String.Format("{0} x {1}", size.Width, size.Height);
        }

        /// <summary>
        /// Tells if the given path equals to parent or subdirectory of parent.
        /// </summary>
        /// <returns></returns>
        public static bool IsPathOrSubDir(string parent, string path)
        {
            Uri baseUri = new Uri(parent + Path.DirectorySeparatorChar);
            Uri pathUri = new Uri(Path.Combine(parent, path) + Path.DirectorySeparatorChar);
            return baseUri.IsBaseOf(pathUri);
        }

        /// <summary>
        /// Tries to parse version string, detecting and IGNORING any additional characters
        /// appended after standard version format.Returns null in case of parsing exception.
        /// </summary>
        public static System.Version TryParseVersion(string s)
        {
            System.Version v = null;
            try
            {
                int pos;
                for (pos = 0; pos < s.Length && (Char.IsDigit(s[pos]) || s[pos] == '.'); pos++);
                s = s.Substring(0, pos);
                v = new System.Version(s);
            }
            catch (Exception){ /* just return null in case of any exception */ }
            return v;
        }
    }
}
