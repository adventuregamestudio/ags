using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class PreCompileGameEventArgs
    {
        private bool _forceRebuild;

        public PreCompileGameEventArgs(bool forceRebuild)
        {
            _forceRebuild = forceRebuild;
            AllowCompilation = true;
        }

        public bool ForceRebuild
        {
            get { return _forceRebuild; }
        }

        public bool AllowCompilation;
        public CompileMessages Errors;
    }
}
