using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum SkipSpeechStyle
    {
        [Description("Mouse, keyboard or timer")]
        MouseOrKeyboardOrTimer = 0,
        [Description("Keyboard only")]
        KeyboardOnly = 1,
        [Description("Timer only")]
        TimerOnly = 2,
        [Description("Mouse or keyboard")]
        MouseOrKeyboard = 3,
        [Description("Mouse only")]
        MouseOnly = 4
    }
}
