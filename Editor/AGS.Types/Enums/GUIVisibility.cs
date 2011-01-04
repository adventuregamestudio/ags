using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUIVisibility
    {
        [Description("Normal, initially on")]
        Normal = 0,
        [Description("When mouse moves to top of screen")]
        MouseYPos = 1,
        [Description("Pause game when shown")]
        PopupModal = 2,
        [Description("Always shown")]
        Persistent = 3,
        [Description("Normal, initially off")]
        NormalButInitiallyOff = 4
    }
}
