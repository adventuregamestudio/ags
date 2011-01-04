using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public enum SpriteImportMethod
    {
        Pixel0 = 0,
        TopLeft,
        BottomLeft,
        TopRight,
        BottomRight,
		LeaveAsIs,
		NoTransparency
    }
}
