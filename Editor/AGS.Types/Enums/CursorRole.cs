using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum CursorRole
    {
        [Description("Role: None")]
        None = -1,
        [Description("Role: Walk to")]
        Walk = 0,
        [Description("Role: Look at")]
        Look = 1,
        [Description("Role: Interact")]
        Interact = 2,
        [Description("Role: Use inventory")]
        UseInv = 4,
        [Description("Role: Pointer")]
        Pointer = 6,
        [Description("Role: Wait")]
        Wait = 7
    }
}
