using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace AGS.Types
{
    public interface IColorMapper
    {
        int MapRgbColorToAgsColourNumber(Color rgbColor);
        Color MapAgsColourNumberToRgbColor(int agsColourNumber);
    }
}
