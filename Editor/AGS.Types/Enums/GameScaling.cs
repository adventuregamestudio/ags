using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum GameScaling
    {
        [Description("Custom round multiplier")]
        Integer,
        [Description("Max round multiplier")]
        MaxInteger,
        [Description("Fill whole screen")]
        StretchToFit,
        [Description("Stretch, preserving aspect ratio")]
        ProportionalStretch
    }
}
