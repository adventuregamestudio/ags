using System.ComponentModel;

namespace AGS.Types
{
    public enum ScreenRotationMode
    {
        [Description("Player can freely rotate the screen if possible")]
        Unlocked = 0,
        [Description("Lock in Portrait")]
        Portrait = 1,
        [Description("Lock in Landscape")]
        Landscape = 2
    }
}