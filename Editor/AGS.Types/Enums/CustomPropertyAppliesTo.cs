using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    [Flags]
    public enum CustomPropertyAppliesTo
    {
        None            = 0,
        Characters      = 0x00000001,
        Hotspots        = 0x00000002,
        InventoryItems  = 0x00000004,
        Objects         = 0x00000008,
        Rooms           = 0x00000010,
        Everything      = 0x0FFFFFFF
    }
}
