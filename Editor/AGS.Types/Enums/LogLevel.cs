using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace AGS.Types.Enums
{
    public enum LogLevel
    {        
        [Description("0 - None")]
        None = 0,
        [Description("1 - Alert")]
        Alert,
        [Description("2 - Fatal")]
        Fatal,
        [Description("3 - Error")]
        Error,
        [Description("4 - Warning")]
        Warn,
        [Description("5 - Info")]
        Info,
        [Description("6 - Debug")]
        Debug,
        [Browsable(false)]
        NumLevels
    }
}
