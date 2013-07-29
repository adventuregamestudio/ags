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
        [Description("Keyboard or timer")]
        // NOTE: can't change constant name due project serialization specifics
        // (this will be possible in 3.4.0 where the value converters are implemented)
        KeyboardOnly = 1,
        [Description("Timer only")]
        TimerOnly = 2,
        [Description("Mouse or keyboard")]
        MouseOrKeyboard = 3,
        [Description("Mouse or timer")]
        MouseOnly = 4,
        [Description("Keyboard only")]
        KeyboardOnlyStrict = 5,
        [Description("Mouse only")]
        MouseOnlyStrict = 6
    }
}
