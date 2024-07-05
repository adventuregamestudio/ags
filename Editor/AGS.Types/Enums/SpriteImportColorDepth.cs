using System.ComponentModel;

namespace AGS.Types
{
    public enum SpriteImportColorDepth
    {
        [Description("Convert to the game's color depth")]
        GameDefault = 0,
        [Description("Import as a 8-bit image")]
        Indexed8Bit = 8
    }
}
