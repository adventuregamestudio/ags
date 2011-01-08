using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace AGS.Types
{
    public struct AGSColor
    {
        public static IColorMapper ColorMapper { get; set; }

        private int _agsColorNumber;

        public AGSColor(int agsColorNumber)
        {
            _agsColorNumber = agsColorNumber;
        }

        public AGSColor(Color rgbColor)
        {
            _agsColorNumber = ColorMapper.MapRgbColorToAgsColourNumber(rgbColor);
        }

        public int ColorNumber { get { return _agsColorNumber; } }

        public Color ToRgb()
        {
            return ColorMapper.MapAgsColourNumberToRgbColor(_agsColorNumber);
        }

    }
}
