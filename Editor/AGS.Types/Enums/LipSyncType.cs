using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum LipSyncType
    {
        [Description("Disabled")]
        None = 0,
        [Description("Text (automatic)")]
        Text = 1,
        [Description("Voice (pamela sync files)")]
        PamelaVoiceFiles = 2
    }
}
