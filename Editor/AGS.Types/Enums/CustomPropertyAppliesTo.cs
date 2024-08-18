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
        AudioClips      = 0x00000020,
        Dialogs         = 0x00000040,
        GUIs            = 0x00000080,
        Regions         = 0x00000100,
        WalkableAreas   = 0x00000200,
        GUIControls     = 0x00000400,
        Everything      = 0x0FFFFFFF
    }
}
