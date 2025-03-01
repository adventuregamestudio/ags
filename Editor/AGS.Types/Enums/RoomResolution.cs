using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum RoomResolution
    {
        [Description("Real")]
        Real = 0,
        [Description("320x200, 320x240")]
        LowRes = 1,
        [Description("640x400, 640x480, 800x600")]
        HighRes = 2,
        [Description("960x600, 1024x768")]
        VeryHighRes = 3
    }
}
