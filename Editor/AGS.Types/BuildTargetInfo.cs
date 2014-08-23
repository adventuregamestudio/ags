using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Types
{
    public class BuildTargetInfo
    {
        public const string COMPILED_DIR = "Compiled";
        public const string WINDOWS_DIR = "Windows";
        public const string LINUX_DIR = "Linux";
        public const string LINUX_LIB32_DIR = "lib32";
        public const string LINUX_LIB64_DIR = "lib64";
        public const string LINUX_LICENSES_DIR = "licenses";

        /// <summary>
        /// Returns required libraries and their paths for the specified platform.
        /// </summary>
        public static Dictionary<string, string> GetRequiredLibraryPaths(BuildTargetPlatform platform)
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
            switch (platform)
            {
                case BuildTargetPlatform.Windows:
                    paths.Add("acwin.exe", Utilities.EditorDirectory);
                    break;
                case BuildTargetPlatform.Linux:
                    string[] libs =
                    {
                        "alleg-alsadigi.so",
                        "alleg-alsamidi.so",
                        "libaldmb.so.1",
                        "liballeg.so.4.4",
                        "libdumb.so.1",
                        "libfreetype.so.6",
                        "libogg.so.0",
                        "libtheora.so.0",
                        "libvorbis.so.0",
                        "libvorbisfile.so.3",
                        "modules.lst"
                    };
                    string[] licenses =
                    {
                        "ags-copyright",
                        "liballegro4.4-copyright",
                        "libdumb1-copyright",
                        "libfreetype6-copyright",
                        "libogg0-copyright",
                        "libtheora0-copyright",
                        "libvorbis0a-copyright"
                    };
                    string linuxDir = Path.Combine(Utilities.EditorDirectory, LINUX_DIR);
                    foreach (string lib in libs)
                    {
                        paths.Add(Path.Combine(LINUX_LIB32_DIR, lib), linuxDir);
                        paths.Add(Path.Combine(LINUX_LIB64_DIR, lib), linuxDir);
                    }
                    foreach (string lic in licenses)
                    {
                        paths.Add(Path.Combine(LINUX_LICENSES_DIR, lic), linuxDir);
                    }
                    paths.Add("ags32", linuxDir);
                    paths.Add("ags64", linuxDir);
                    break;
                default:
                    break;
            };
            return paths;
        }

        /// <summary>
        /// Helper function for FlagsEditorControl.ValueExclusionCheck delegate.
        /// </summary>
        public static bool IsBuildTargetAvailable(Enum e)
        {
            if (e.GetType() != typeof(BuildTargetPlatform)) return false;
            return IsBuildTargetAvailable((BuildTargetPlatform)e);
        }

        /// <summary>
        /// Returns whether all files are available to build targeting the specified platform.
        /// </summary>
        public static bool IsBuildTargetAvailable(BuildTargetPlatform platform)
        {
            Dictionary<string, string> paths = GetRequiredLibraryPaths(platform);
            foreach (KeyValuePair<string, string> pair in paths)
            {
                string fullPath = pair.Value;
                if (!fullPath.EndsWith(pair.Key)) fullPath = Path.Combine(fullPath, pair.Key);
                if (!File.Exists(fullPath)) return false;
            }
            return true;
        }

        /// <summary>
        /// Helper function to join parts of a path in the Compiled folder.
        /// </summary>
        public static string GetCompiledPath(BuildTargetPlatform platform, params string[] parts)
        {
            string platformDir = WINDOWS_DIR;
            switch (platform)
            {
                case BuildTargetPlatform.Linux:
                    platformDir = LINUX_DIR;
                    break;
                default:
                    break;
            };
            StringBuilder sb = new StringBuilder(COMPILED_DIR);
            for (int i = 0; i < parts.Length; ++i)
            {
                if (((i == 0) && (parts[0] == COMPILED_DIR)) ||
                    ((i == 1) && (parts[1] == platformDir))) continue;
                sb.Append(Path.DirectorySeparatorChar + parts[i]);
            }
            return sb.ToString();
        }
    }
}
