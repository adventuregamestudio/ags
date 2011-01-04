using System.ComponentModel;

namespace AGS.Types
{
    public enum InventoryHotspotMarkerStyle
    {
        [Description("None")]
        None = 0,
        [Description("Draw crosshair")]
        Crosshair = 1,
        [Description("Use crosshair sprite")]
        Sprite = 2
    }
}
