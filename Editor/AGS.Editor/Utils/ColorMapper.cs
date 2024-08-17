using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using System.Drawing;

namespace AGS.Editor
{
    public class ColorMapper : IColorMapper
    {
        private AGSEditor _editor;

        public ColorMapper(AGSEditor editor)
        {
            _editor = editor;
        }

        /// <summary>
        /// Lookup table for scaling 5 bit colors up to 8 bits,
        /// copied from Allegro 4 library in order to match Editor and Engine.
        /// </summary>
        private readonly static int[] RGBScale5 = new int[32]
        {
           0,   8,   16,  24,  33,  41,  49,  57,
           66,  74,  82,  90,  99,  107, 115, 123,
           132, 140, 148, 156, 165, 173, 181, 189,
           198, 206, 214, 222, 231, 239, 247, 255
        };


        /// <summary>
        /// Lookup table for scaling 6 bit colors up to 8 bits,
        /// copied from Allegro 4 library in order to match Editor and Engine.
        /// </summary>
        private readonly static int[] RGBScale6 = new int[64]
        {
           0,   4,   8,   12,  16,  20,  24,  28,
           32,  36,  40,  44,  48,  52,  56,  60,
           65,  69,  73,  77,  81,  85,  89,  93,
           97,  101, 105, 109, 113, 117, 121, 125,
           130, 134, 138, 142, 146, 150, 154, 158,
           162, 166, 170, 174, 178, 182, 186, 190,
           195, 199, 203, 207, 211, 215, 219, 223,
           227, 231, 235, 239, 243, 247, 251, 255
        };

        #region IColorMapper

        public int MapRgbColorToAgsColourNumber(Color rgbColor)
        {
            // For 8-bit games find the nearest matching palette index
            if (_editor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette)
            {
                return FindNearestColourInGamePalette(rgbColor);
            }
            // Otherwise compose a 32-bit xRGB
            return ColorToAgsColourNumberDirect(rgbColor);
        }

        public Color MapAgsColourNumberToRgbColor(int agsColorNumber)
        {
            // For 8-bit games treat the color number as a palette index
            if (_editor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette)
            {
                if (agsColorNumber >= _editor.CurrentGame.Palette.Length)
                {
                    // don't attempt to map invalid 8-bit colour numbers >255
                    return Color.Black;
                }
                return _editor.CurrentGame.Palette[agsColorNumber].Colour;
            }
            // Otherwise treat number as a 32-bit xRGB
            return AgsColourNumberToColorDirect(agsColorNumber);
        }

        #endregion

        /// <summary>
        /// Converts RGB Color to a AGS color number directly, without use of palette or "special" entries.
        /// </summary>
        public int ColorToAgsColourNumberDirect(Color color)
        {
            return color.B | (color.G << 8) | (color.R << 16);
        }

        /// <summary>
        /// Converts AGS color number to RGB Color directly, without use of palette or "special" entries.
        /// </summary>
        public Color AgsColourNumberToColorDirect(int agsColorNumber)
        {
            return Color.FromArgb(
                (agsColorNumber >> 16) & 0xff,
                (agsColorNumber >> 8) & 0xff,
                agsColorNumber & 0xff);
        }

        private int FindNearestColourInGamePalette(Color rgbColor)
        {
            int nearestDistance = int.MaxValue;
            int nearestIndex = 0;

            foreach (PaletteEntry entry in _editor.CurrentGame.Palette)
            {
                int thisDistance = Math.Abs(entry.Colour.R - rgbColor.R) +
                                   Math.Abs(entry.Colour.G - rgbColor.G) +
                                   Math.Abs(entry.Colour.B - rgbColor.B);

                if (thisDistance < nearestDistance)
                {
                    nearestDistance = thisDistance;
                    nearestIndex = entry.Index;
                }
            }

            return nearestIndex;
        }
    }
}
