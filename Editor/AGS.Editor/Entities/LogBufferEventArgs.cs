using System;

namespace AGS.Editor
{
    /// <summary>
    /// Notifies of the most recent LogBuffer changes.
    /// Tells whether buffer has been reset (either cleared or applied a filter),
    /// how many characters were popped (discarded) and pushed (added).
    /// </summary>
    public class LogBufferEventArgs : EventArgs
    {
        public LogBufferEventArgs(bool reset, int popCount, int pushCount)
        {
            Reset = reset;
            PopCount = popCount;
            PushCount = pushCount;
        }

        public readonly bool Reset;
        public readonly int PopCount;
        public readonly int PushCount;
    }
}
