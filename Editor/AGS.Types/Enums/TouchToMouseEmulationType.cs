using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum TouchToMouseEmulationType
    {
        [Description("Off")]
        Off = 0,
        [Description("One finger (touch is left button down)")]
        OneFinger = 1,
        [Description("Two fingers (tap to click)")]
        TwoFingers = 2
    }
}