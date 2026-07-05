using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUITextBoxKeyClaimStyle
    {
        [Description("Always")]
        Always  = 0,
        [Description("Handled input events (key and text)")]
        Handled = 1,
        [Description("Text input events only")]
        TextOnly = 2,
    }
}
