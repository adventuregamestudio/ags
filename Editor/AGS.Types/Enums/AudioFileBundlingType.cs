using System.ComponentModel;

namespace AGS.Types
{
    public enum AudioFileBundlingType
    {
        [Description("In Game EXE")]
        InGameEXE = 1,
        [Description("In Audio VOX")]
        InSeparateVOX = 2
    }
}
