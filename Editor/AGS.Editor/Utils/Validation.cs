using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace AGS.Editor.Utils
{
    public class Validation
    {
        public static bool FilenameIsValid(string filename)
        {
            return !(filename.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0);
        }

        public static bool PathIsAvailable(string path)
        {
            if (File.Exists(path))
                return false;

            if (Directory.Exists(path))
                return false;

            return true;
        }

        public static bool PathIsAbsoluteDriveLetter(string path)
        {
            return Regex.IsMatch(path, @"^[a-zA-Z]:\\");
        }

        public static bool PathIsAbsolute(string path)
        {
            bool rooted = false;

            try
            {
                rooted = Path.IsPathRooted(path);
            }
            catch (ArgumentException)
            {
                // pass
            }

            return rooted;
        }

        public static bool PathIsValid(string path)
        {
            return !(path.IndexOfAny(Path.GetInvalidPathChars()) >= 0);
        }

        public static bool StringIsAsciiCharactersOnly(string fileName)
        {
            return !fileName.Any(c => c > 127);
        }
    }
}
