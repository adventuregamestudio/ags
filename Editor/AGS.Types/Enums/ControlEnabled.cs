using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum ControlEnabled
    {
        [Description("Default (direct)")]
        Direct = 0,
        [Description("Relative (infinite on desktop)")]
        Relative = 1
    }
}