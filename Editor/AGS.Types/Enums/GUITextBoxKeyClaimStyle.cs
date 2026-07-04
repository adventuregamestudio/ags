using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUITextBoxKeyClaimStyle
    {
        [Description("Always")]
        Always  = 0,
        [Description("Handled input events only")]
        Handled = 1
    }
}
