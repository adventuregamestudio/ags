using System.ComponentModel;

namespace AGS.Types
{
    public enum GameColorDepth
    {
        [Description("8-bit colour")]
        Palette = 1,
        [Description("16-bit (hi-colour)")]
        HighColor = 2,
        [Description("32-bit (true-colour)")]
        TrueColor = 4
    }
}
