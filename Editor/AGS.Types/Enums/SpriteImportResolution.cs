using System.ComponentModel;

namespace AGS.Types
{
    public enum SpriteImportResolution
    {
        [Description("Low (320x240 and below)")]
        LowRes = 0,
        [Description("High (above 320x240)")]
        HighRes = 1
    }
}
