using System;
using System.ComponentModel;
using System.IO;

namespace AGS.Types
{
    public struct Targets
    {
        [Flags]
        public enum Platforms
        {
            Windows = 1,
            Linux = 2,
            Android = 4,
            iOS = 8,
            OSX = 16
        };

        public static string[] GetPlatformRequiredLibraryNames(Targets.Platforms platform)
        {
            if (platform == Platforms.Linux)
            {
                return new string[]
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
            }
            return new string[0];
        }

        /// <summary>
        /// Helper function for FlagsEditorControl.ValueExclusionCheck delegate.
        /// </summary>
        public static bool IsPlatformAvailable(Enum platform)
        {
            if (platform.GetType() != typeof(Targets.Platforms)) return false;
            return IsPlatformAvailable((Targets.Platforms)platform);
        }

        /// <summary>
        /// Checks whether the specified platform is currently available for building.
        /// </summary>
        public static bool IsPlatformAvailable(Targets.Platforms platform)
        {
            if (platform == Platforms.Windows) return true;
            string editorDir = Path.GetDirectoryName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName);
            if (platform == Targets.Platforms.Linux)
            {
                string linuxDir = Path.Combine(editorDir, "Linux");
                string linuxLib32Dir = Path.Combine(linuxDir, "lib32");
                string linuxLib64Dir = Path.Combine(linuxDir, "lib64");
                string linuxAGS32Path = Path.Combine(linuxDir, "ags32");
                string linuxAGS64Path = Path.Combine(linuxDir, "ags64");
                string[] linuxLibs = GetPlatformRequiredLibraryNames(Platforms.Linux);
                if ((!Directory.Exists(linuxDir)) || (!Directory.Exists(linuxLib32Dir)) ||
                    (!Directory.Exists(linuxLib64Dir)) || (!File.Exists(linuxAGS32Path)) ||
                    (!File.Exists(linuxAGS64Path))) return false;
                foreach (string lib in linuxLibs)
                {
                    if ((!File.Exists(Path.Combine(linuxLib32Dir, lib))) ||
                        (!File.Exists(Path.Combine(linuxLib64Dir, lib)))) return false;
                }
                return true;
            }
            return false;
        }

        public static Platforms GetAvailablePlatforms()
        {
            return
                Platforms.Windows | // Windows is always available
                (IsPlatformAvailable(Platforms.Linux) ? Platforms.Linux : 0) |
                (IsPlatformAvailable(Platforms.Android) ? Platforms.Android : 0) |
                (IsPlatformAvailable(Platforms.iOS) ? Platforms.iOS : 0) |
                (IsPlatformAvailable(Platforms.OSX) ? Platforms.OSX : 0);
        }
    }
}
