using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    [FlagsAttribute]
    enum AlignmentFlags
    {
        Left    = 0x01,
        Right   = 0x02,
        HCenter = 0x04,
        Top     = 0x10,
        Bottom  = 0x20,
        VCenter = 0x40
    }
}
