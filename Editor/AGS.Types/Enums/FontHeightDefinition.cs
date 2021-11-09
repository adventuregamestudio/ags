using System;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum FontHeightDefinition
    {
        [Description("Nominal height (point size)")]
        NominalHeight,
        [Description("Real pixel height")]
        PixelHeight
    }
}
