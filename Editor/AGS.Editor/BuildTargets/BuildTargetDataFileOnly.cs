using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetDataFileOnly : BuildTarget
    {
        public override bool Available
        {
            get
            {
                return true; // data file is always available
            }
        }

        public override bool IsTargetForBuild
        {
            get
            {
                return true; // data file is always a target
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return "Compiled";
            }
        }

        public override BuildTargetPlatform Platform
        {
            get
            {
                return BuildTargetPlatform.DataFileOnly;
            }
        }

        public override void EnsureStandardSubfoldersExist()
        {
            Directory.CreateDirectory(OutputDirectoryFullPath); // automatically checks if folder exists
        }
    }
}
