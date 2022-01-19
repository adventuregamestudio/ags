using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace AGS.Types.Enums
{
    public enum LogGroup
    {
        [Description("None")]
        None = -1,
        [Description("Main")]
        Main = 0,
        [Description("Game")]
        Game,
        [Description("Script")]
        Script,
        [Description("Sprite Cache")]
        SprCache,
        [Description("Managed Objects")]
        ManObj,
        [Description("SDL")]
        SDL,
        [Browsable(false)]
        NumGroups
    }
}
