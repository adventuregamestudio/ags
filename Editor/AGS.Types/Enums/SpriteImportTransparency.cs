using System.ComponentModel;

namespace AGS.Types
{
    public enum SpriteImportTransparency
    {
        [Description("Pixels of index 0 will be transparent (256-colour games only)")]
        PaletteIndex0,
        [Description("The top-left pixel will be the transparent colour for this sprite")]
        TopLeft,
        [Description("The bottom-left pixel will be the transparent colour for this sprite")]
        BottomLeft,
        [Description("The top-right pixel will be the transparent colour for this sprite")]
        TopRight,
        [Description("The bottom-right pixel will be the transparent colour for this sprite")]
        BottomRight,
        [Description("AGS will leave the sprite's pixels as they are. Any pixels that match the AGS Transparent Colour will be invisible.")]
        LeaveAsIs,
        [Description("AGS will remove all transparent pixels by changing them to a very similar non-transparent colour")]
        NoTransparency
    }
}
