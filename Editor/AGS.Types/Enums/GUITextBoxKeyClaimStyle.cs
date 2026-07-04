using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUITextBoxKeyClaimStyle
    {
        [Description("Always")]
        Always  = 0,
        [Description("Never")]
        Never   = 1,
        [Description("Handled keys only")]
        Handled = 2
    }
}
