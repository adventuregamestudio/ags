using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// A generic event args with CompileMessages list.
    /// May be used as a parent for extended Args classes.
    /// </summary>
    public class GenericMessagesArgs
    {
        /// <summary>
        /// Output messages which may be filled by the compilation step handler.
        /// </summary>
        public CompileMessages Messages { get; private set; }

        public GenericMessagesArgs(CompileMessages messages)
        {
            Messages = messages;
        }
    }
}
