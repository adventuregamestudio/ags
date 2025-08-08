﻿using System;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum FontMetricsFixup
    {
        [Description("Do nothing")]
        None,
        [Obsolete]
        [Description("Resize ascender to the nominal font height")]
        SetAscenderToHeight
    }
}
