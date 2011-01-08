using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    public class GetScriptEditorControlEventArgs
    {
        public GetScriptEditorControlEventArgs(string scriptFileName, bool showEditor)
        {
            this.ScriptFileName = scriptFileName;
            this.ShowEditor = showEditor;
        }

        public string ScriptFileName { get; private set; }
        public IScriptEditorControl ScriptEditor { get; set; }
        public bool ShowEditor { get; set; }
    }
}
