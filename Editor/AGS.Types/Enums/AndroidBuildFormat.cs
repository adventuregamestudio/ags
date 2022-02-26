using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum AndroidBuildFormat
    {
        [Description("Apk (Embedded Game)")]
        ApkEmbedded = 0,
        [Description("AAB (Embedded Game)")]
        AabEmbedded = 1,
        [Description("AAB (Game as Asset)")]
        Aab = 2
    }
}
