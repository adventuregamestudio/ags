using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum GameResolutions
    {
        [Description("320 x 200")]
        R320x200 = 1,
        [Description("320 x 240")]
        R320x240 = 2,
        [Description("640 x 400")]
        R640x400 = 3,
        [Description("640 x 480")]
        R640x480 = 4,
        [Description("800 x 600")]
        R800x600 = 5,
        [Description("1024 x 768")]
        R1024x768 = 6,
        [Description("1280 x 720")]
        R1280x720 = 7,
        Custom    = 8
    }
}
