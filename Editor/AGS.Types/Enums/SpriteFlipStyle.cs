using System;
using System.ComponentModel;

namespace AGS.Types
{
    [Flags]
    [DefaultValue(None)]
    public enum SpriteFlipStyle
    {
        None         = 0,
        Horizontal   = 1,
        Vertical     = 2,
        Both         = Horizontal | Vertical
    }
}
