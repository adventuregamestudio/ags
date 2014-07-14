using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum GraphicsDriver
    {
        [Description("DirectDraw 5 (default)")]
        DX5,
        [Description("Direct3D 9 hardware acceleration")]
        D3D9
    }
}
