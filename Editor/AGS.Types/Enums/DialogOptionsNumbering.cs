using System;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    [DeserializeConvertValue("False", "KeyShortcutsOnly")]
    [DeserializeConvertValue("True", "Normal")]
    public enum DialogOptionsNumbering
    {
        [Description("Disable")]
        None = -1,
        [Description("Keyboard shortcuts only")]
        KeyShortcutsOnly = 0,
        [Description("Draw numbers and use keyboard shortcuts")]
        Normal = 1,
    }
}
