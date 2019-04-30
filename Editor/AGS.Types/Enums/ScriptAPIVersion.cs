using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Defines a version of Script API, corresponding to particular AGS release
    /// </summary>
    public enum ScriptAPIVersion
    {
        [Description("3.2.1")]
        v321 = 0,
        [Description("3.3.0")]
        v330 = 1,
        [Description("3.3.4")]
        v334 = 2,
        [Description("3.3.5")]
        v335 = 3,
        [Description("3.4.0")]
        v340 = 4,
        [Description("3.4.1")]
        v341 = 5,
        [Description("3.5.0 Alpha")]
        v350 = 6,
        [Description("3.5.0 Final")]
        v3507 = 7,
        // Highest constant is used for automatic upgrade to new API when
        // the game is loaded in the newer version of the Editor
        [Description("Highest")]
        Highest = Int32.MaxValue,
    }
}
