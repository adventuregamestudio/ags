using AGS.Types;
using System;

namespace AGS.Editor
{
    public class OutputCreationStepArgs : GenericMessagesArgs
    {
        public bool MiniExeForDebug { get; private set; }

        public OutputCreationStepArgs(bool miniExeForDebug, CompileMessages messages)
            : base(messages)
        {
            MiniExeForDebug = miniExeForDebug;
        }
    }
}
