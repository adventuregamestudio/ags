using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum GameColorDepth
    {
        [Description("8-bit colour")]
        Palette = 1,
        [Obsolete]
        [Browsable(false)]
        [Description("16-bit (hi-colour)")]
        HighColor = 2,
        [Description("32-bit (true-colour)")]
        TrueColor = 4
    }
}
