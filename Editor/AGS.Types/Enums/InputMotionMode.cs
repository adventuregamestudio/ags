using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum InputMotionMode
    {
        [Description("Direct (capture device position)")]
        Direct = 0,
        [Description("Relative (capture device movement)")]
        Relative = 1
    }
}