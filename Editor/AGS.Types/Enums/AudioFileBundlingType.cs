using System.ComponentModel;

namespace AGS.Types
{
    public enum AudioFileBundlingType
    {
        [Description("In Main Game Data")]
        InGameEXE = 1,
        [Description("In Audio VOX")]
        InSeparateVOX = 2
    }
}
