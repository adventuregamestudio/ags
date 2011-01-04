using System.ComponentModel;

namespace AGS.Types
{
    public enum SpriteImportResolution
    {
        [Description("320x200, 320x240")]
        LowRes = 0,
        [Description("640x400, 640x480, 800x600")]
        HighRes = 1
    }
}
