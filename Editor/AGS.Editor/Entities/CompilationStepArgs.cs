using System;
using AGS.Types;

namespace AGS.Editor
{
    public class CompilationStepArgs : GenericMessagesArgs
    {
        /// <summary>
        /// A list of scripts prepared for compilation.
        /// </summary>
        public ScriptsAndHeaders ScriptsToCompile { get; private set; }

        public CompilationStepArgs(ScriptsAndHeaders scripts, CompileMessages messages)
            : base(messages)
        {
            ScriptsToCompile = scripts;
        }
    }
}
