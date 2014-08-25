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
        DataFileOnly = 0x00,
        Windows      = 0x02,
        Linux        = 0x04,
        Android      = 0x08,
        iOS          = 0x10,
        OSX          = 0x20
    };
}
