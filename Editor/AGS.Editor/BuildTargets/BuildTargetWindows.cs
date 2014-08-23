using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetWindows : BuildTarget
    {
        public override bool IsTargetForBuild
        {
            get
            {
                // TODO: check current target in Settings
                throw new NotImplementedException();
            }
        }

        public override string OutputDirectory
        {
            get { throw new NotImplementedException(); }
        }

        public override BuildTargetPlatform Platform
        {
            get { return BuildTargetPlatform.Windows; }
        }

        public override void EnsureStandardSubfoldersExist()
        {
            // TODO: check what targets are selected to choose between "Compiled" and "Compiled/Windows"
            throw new NotImplementedException();
        }
    }
}
