using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetLinux : BuildTargetBase
    {
        public const string LINUX_DIR = "Linux";
        public const string LINUX_LIB32_DIR = "lib32";
        public const string LINUX_LIB64_DIR = "lib64";
        public const string LINUX_LICENSES_DIR = "licenses";
        public const string LINUX_DATA_DIR = "data";

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
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
            string linuxDir = Path.Combine(AGSEditor.Instance.EditorDirectory, LINUX_DIR);
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
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[]
            {
                GetCompiledPath(LINUX_LIB32_DIR),
                GetCompiledPath(LINUX_LIB64_DIR),
                GetCompiledPath(LINUX_LICENSES_DIR)
            };
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            throw new NotImplementedException();
        }

        public override string OutputDirectory
        {
            get
            {
                return LINUX_DIR;
            }
        }

        public override BuildTargetPlatform Platform
        {
            get
            {
                return BuildTargetPlatform.Linux;
            }
        }
    }
}
