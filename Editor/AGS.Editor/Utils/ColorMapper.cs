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

        #region IColorMapper

        public int MapRgbColorToAgsColourNumber(Color rgbColor)
        {
            int green = rgbColor.G;

            if (rgbColor.R == 0 && rgbColor.G == 0 && rgbColor.B > 0)
            {
                // make sure the colour number doesn't end up being a special EGA colour
                green = 4;
            }
            else if (_editor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette)
            {
                return FindNearestColourInGamePalette(rgbColor);
            }

            return (rgbColor.B / 8) + ((green / 4) << 5) + ((rgbColor.R / 8) << 11);
        }

        public Color MapAgsColourNumberToRgbColor(int agsColorNumber)
        {
            int red = ((agsColorNumber >> 11) * 8) & 255;
            int green = ((agsColorNumber >> 5) * 4) & 255;
            int blue = ((agsColorNumber) * 8) & 255;
            if (((agsColorNumber > 0) && (agsColorNumber < 32)) ||
                (_editor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette))
            {
                if (agsColorNumber >= _editor.CurrentGame.Palette.Length)
                {
                    // don't attempt to map invalid 8-bit colour numbers >255
                    return Color.Black;
                }
                // Special Color Number that translates to one of the EGA colours
                return _editor.CurrentGame.Palette[agsColorNumber].Colour;
            }
            return Color.FromArgb(red, green, blue);
        }

        #endregion

        /// <summary>
        /// Converts RGB Color to a AGS color number directly, without use of palette or "special" entries.
        /// </summary>
        public int ColorToAgsColourNumberDirect(Color color)
        {
            return (color.B / 8) + ((color.G / 4) << 5) + ((color.R / 8) << 11);
        }

        /// <summary>
        /// Converts AGS color number to RGB Color directly, without use of palette or "special" entries.
        /// </summary>
        public Color AgsColourNumberToColorDirect(int agsColorNumber)
        {
            return Color.FromArgb(
                (agsColorNumber >> 11) * 8,
                ((agsColorNumber >> 5) & 0x3f) * 4,
                (agsColorNumber & 0x1f) * 8);
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
