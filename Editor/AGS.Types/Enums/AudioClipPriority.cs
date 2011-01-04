using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// This needs to match the enum definition in agsdefns.sh
    /// </summary>
    public enum AudioClipPriority
    {
        Inherit = -1,
        VeryLow = 1,
        Low = 25,
        Normal = 50,
        High = 75,
        VeryHigh = 100
    }
}
