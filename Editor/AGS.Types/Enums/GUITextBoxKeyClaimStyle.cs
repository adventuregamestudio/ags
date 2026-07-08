using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum GUITextBoxKeyClaimStyle
    {
        [Description("All key and text input events")]
        All         = 0,
        [Description("Handled input events (key and text)")]
        Handled     = 1,
        [Description("Text input events only")]
        TextOnly    = 2,
    }
}
