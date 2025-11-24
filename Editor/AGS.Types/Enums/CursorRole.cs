using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum CursorRole
    {
        None = -1,
        [Description("Walk to")]
        Walk = 0,
        [Description("Look at")]
        Look = 1,
        [Description("Interact")]
        Interact = 2,
        [Description("Use inventory")]
        UseInv = 4,
        [Description("Pointer")]
        Pointer = 6,
        [Description("Wait")]
        Wait = 7
    }
}
