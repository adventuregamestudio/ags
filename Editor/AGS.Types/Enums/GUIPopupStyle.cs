using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUIPopupStyle
    {
        [Description("Normal")]
        Normal = 0,
        [Description("When mouse moves to top of screen")]
        MouseYPos = 1,
        [Description("Pause game when shown")]
        PopupModal = 2,
        [Description("Always shown")]
        Persistent = 3
    }
}
