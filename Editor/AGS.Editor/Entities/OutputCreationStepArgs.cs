using System;

namespace AGS.Editor
{
    public class OutputCreationStepArgs
    {
        public bool MiniExeForDebug { get; private set; }

        public OutputCreationStepArgs(bool miniExeForDebug)
        {
            MiniExeForDebug = miniExeForDebug;
        }
    }
}
