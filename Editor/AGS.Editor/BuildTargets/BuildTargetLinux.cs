using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetLinux : BuildTarget
    {
        public override bool IsTargetForBuild
        {
            get { throw new NotImplementedException(); }
        }

        public override string OutputDirectory
        {
            get { return Path.Combine(BuildTargetInfo.COMPILED_DIR, BuildTargetInfo.LINUX_DIR); }
        }

        public override BuildTargetPlatform Platform
        {
            get { return BuildTargetPlatform.Linux; }
        }

        public override void EnsureStandardSubfoldersExist()
        {
            string[] folders =
            {
                BuildTargetInfo.GetCompiledPath(BuildTargetPlatform.Linux, BuildTargetInfo.LINUX_LIB32_DIR),
                BuildTargetInfo.GetCompiledPath(BuildTargetPlatform.Linux, BuildTargetInfo.LINUX_LIB64_DIR),
                BuildTargetInfo.GetCompiledPath(BuildTargetPlatform.Linux, BuildTargetInfo.LINUX_LICENSES_DIR)
            };
            foreach (string folder in folders)
            {
                Directory.CreateDirectory(Utilities.GetFullPathFromProjectRelative(folder)); // automatically checks if folder exists
            }
        }
    }
}
