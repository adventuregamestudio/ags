using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Enum representing all known platforms.
    /// </summary>
    [Flags]
    public enum BuildTargetPlatform
    {
        DataFileOnly = 1,
        Windows      = 2,
        Linux        = 4,
        Android      = 8,
        iOS          = 16,
        OSX          = 32
    };
}
