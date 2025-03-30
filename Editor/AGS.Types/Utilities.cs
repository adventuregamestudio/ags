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
        /// <summary>
        /// A UTF-8 Encoding without BOM.
        /// Unfortunately, default UTF8 encoding provided by .NET is with BOM,
        /// so we define our own to refer to whenever we need to save files in utf-8.
        /// </summary>
        public static Encoding UTF8 = new UTF8Encoding(false);

        public static void Swap<T>(this IList<T> list, int index1, int index2)
        {
            if (index1 == index2)
                return;

            var temp = list[index1];
            list[index1] = list[index2];
            list[index2] = temp;
        }

        public static T GetDefaultValue<T>(Type type, string propertyName, T defaultValue)
        {
            PropertyInfo property = type.GetProperty(propertyName);
            DefaultValueAttribute[] defaults = (DefaultValueAttribute[])property.GetCustomAttributes(typeof(DefaultValueAttribute), false);
            return (defaults.Length > 0) ? (T)defaults[0].Value : defaultValue;
        }

        /// <summary>
        /// Gets Description attribute of the enum value; if not available returns value name.
        /// </summary>
        public static string GetEnumValueDescription<T>(T enumValue)
        {
            foreach (System.Reflection.FieldInfo fieldInfo in typeof(T).GetFields())
            {
                if (fieldInfo.Name == Enum.GetName(typeof(T), enumValue))
                {
                    object[] attributes = fieldInfo.GetCustomAttributes(typeof(DescriptionAttribute), true);
                    if (attributes.Length > 0)
                    {
                        return ((DescriptionAttribute)attributes[0]).Description;
                    }
                }
            }

            return Enum.GetName(typeof(T), enumValue);
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
                if (name[i].IsScriptWordChar())
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

        /// <summary>
        /// Tells if the character is a valid symbol for a script keyword or a variable name.
        /// </summary>
        public static bool IsScriptWordChar(this Char theChar)
        {
            return ((theChar >= 'A') && (theChar <= 'Z')) ||
                ((theChar >= 'a') && (theChar <= 'z')) ||
                ((theChar >= '0') && (theChar <= '9')) ||
                (theChar == '_');
        }

        public static string ValidateScriptName(string name, int truncateToLength)
        {
            for (int i = 0; i < name.Length; i++)
            {
                if ((i == 0) && (Char.IsDigit(name[i])))
                {
                    throw new InvalidDataException("Script name cannot start with number: " + name);
                }
                if (!name[i].IsScriptWordChar())
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
            return ValidateScriptName(name, Int32.MaxValue);
        }

        public static string ValidateFilePath(string path)
        {
            path = path.Trim('\"');
            if (!PathIsValid(path))
            {
                throw new InvalidDataException($"The file path contains invalid characters: {path}");
            }
            return path;
        }

        public static bool PathIsValid(string path)
        {
            return !(path.IndexOfAny(Path.GetInvalidPathChars()) >= 0);
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

        public static string ResolutionToAspectRatio(Size size)
        {
            int gcd = GreatestCommonDivisor(size.Width, size.Height);
            int w = size.Width / gcd;
            int h = size.Height / gcd;
            int scale = 1;

            if (w == 8 && h == 5)
            {
                scale = 2;
            }

            return String.Format("{0}:{1}", w * scale, h * scale);
        }

        public static int GreatestCommonDivisor(int a, int b)
        {
            int remainder;

            while (b != 0)
            {
                remainder = a % b;
                a = b;
                b = remainder;
            }

            return a;
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

        public static Encoding EncodingFromName(string name)
        {
            if (string.Compare(name, "ANSI", true) == 0)
                return Encoding.Default;
            else if (string.Compare(name, "UTF-8", true) == 0)
                return Utilities.UTF8; // UTF-8 w/o BOM
            return Encoding.GetEncoding(name);
        }
    }
}
