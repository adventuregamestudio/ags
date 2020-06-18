using System.ComponentModel;

namespace AGS.Types
{
    public enum RoomResolution
    {
        [Description("Real")]
        Real = 0,
        [Description("Low (320x240 and below)")]
        LowRes = 1,
        [Description("High (above 320x240)")]
        HighRes = 2
    }
}
