using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum ScaleNativeResolution
    {
        [Description("User defined")]
        Default = 0,
        [Description("Enabled")]
        True = 1,
        [Description("Disabled")]
        False = 2
    }
}
