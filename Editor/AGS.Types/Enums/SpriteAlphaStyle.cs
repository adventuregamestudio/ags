using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum SpriteAlphaStyle
    {
        [Description("Classic")]
        Classic = 0,
        [Description("Proper Alpha Blending")]
        Improved = 1,
    }
}
