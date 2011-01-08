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
            if ((agsColorNumber > 0) && (agsColorNumber < 32))
            {
                // Special Color Number that translates to one of the EGA colours
                return _editor.CurrentGame.Palette[agsColorNumber].Colour;
            }
            return Color.FromArgb(red, green, blue);
        }

        private int FindNearestColourInGamePalette(Color rgbColor)
        {
            int nearestDistance = 999999;
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
