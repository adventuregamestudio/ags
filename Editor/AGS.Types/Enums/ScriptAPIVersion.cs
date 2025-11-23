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
        [Description("3.5.1")]
        v351 = 8,
        // Define API constant value as AGS version represented as NN,NN,NN,NN:
        // e.g. 3.6.0     is 03,06,00,00 (3060000),
        //      4.12.3.25 is 04,12,03,25 (4120325), and so on
        [Description("3.6.0 Alpha")]
        v360 = 3060000,
        [Description("3.6.0 Final")]
        v36026 = 3060026,
        [Description("3.6.1")]
        v361 = 3060100,
        [Description("3.6.2")]
        v362 = 3060200,
        [Description("3.6.3")]
        v363 = 3060300,
        // TODO: invent better names for 399/400 versions, for user convenience;
        // maybe hide some of the early alpha values from selection?
        [Description("3.99.x Alpha")]
        v399 = 3990000,
        [Description("4.0.0 Alpha 8")]
        v400 = 4000003,
        [Description("4.0.0 Alpha 12")]
        v400_07 = 4000007,
        [Description("4.0.0 Alpha 18")]
        v400_14 = 4000014,
        [Description("4.0.0 Alpha 20")]
        v400_16 = 4000016,
        [Description("4.0.0 Alpha 22")]
        v400_18 = 4000018,
        [Description("4.0.0 Alpha 28")]
        v400_24 = 4000024,
        // Highest constant is used for automatic upgrade to new API when
        // the game is loaded in the newer version of the Editor
        [Description("Latest version")]
        Highest = Int32.MaxValue,
    }
}
