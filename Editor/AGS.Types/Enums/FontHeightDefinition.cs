using System;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum FontHeightDefinition
    {
        [Description("Nominal height (import size)")]
        NominalHeight,
        [Description("Full graphical height")]
        PixelHeight
    }
}
