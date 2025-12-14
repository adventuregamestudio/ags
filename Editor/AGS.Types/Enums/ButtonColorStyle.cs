using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum ButtonColorStyle
    {
        [Description("Default (all colors fixed)")]
        Default,
        [Description("Dynamic 3D (border color fixed)")]
        Dynamic,
        [Description("Dynamic Flat (border color changes, no shadow)")]
        DynamicFlat
    }
}
