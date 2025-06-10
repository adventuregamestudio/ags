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
            // For 8-bit games find the nearest matching palette index
            if (_editor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette)
            {
                return FindNearestColourInGamePalette(rgbColor, _editor.CurrentGame.Palette);
            }
            // Otherwise compose a 32-bit ARGB
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
                    return Color.FromArgb(0);
                }
                return _editor.CurrentGame.Palette[agsColorNumber].Colour;
            }
            // Otherwise treat number as a 32-bit ARGB
            return AgsColourNumberToColorDirect(agsColorNumber);
        }

        #endregion

        /// <summary>
        /// Converts RGB Color to a AGS color number directly, without use of palette or "special" entries.
        /// </summary>
        public static int ColorToAgsColourNumberDirect(Color color)
        {
            return color.B | (color.G << 8) | (color.R << 16) | (color.A << 24);
        }

        /// <summary>
        /// Converts AGS color number to RGB Color directly, without use of palette or "special" entries.
        /// </summary>
        public static Color AgsColourNumberToColorDirect(int agsColorNumber)
        {
            return Color.FromArgb(
                (agsColorNumber >> 24) & 0xff,
                (agsColorNumber >> 16) & 0xff,
                (agsColorNumber >> 8) & 0xff,
                agsColorNumber & 0xff);
        }

        /// <summary>
        /// Finds a palette index which color is closest to the given RGB.
        /// Any non-zero alpha value is ignored; any color with zero alpha is treated as the
        /// same "transparent color" palette entry
        /// </summary>
        public static int FindNearestColourInGamePalette(Color rgbColor, PaletteEntry[] palette)
        {
            // Special case: ARGB colors with zero alpha are converted to transparent entry 0
            if (rgbColor.A == 0)
                return 0;

            int nearestDistance = int.MaxValue;
            int nearestIndex = 0;

            foreach (PaletteEntry entry in palette)
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

        /// <summary>
        /// Remaps a color number between two game depths.
        /// </summary>
        public static int RemapColourNumberToDepth(int colourNumber, PaletteEntry[] palette,
            GameColorDepth gameColorDepth, GameColorDepth oldColorDepth)
        {
            // If depth settings are identical, then simply return same color
            if (gameColorDepth == oldColorDepth)
                return colourNumber;

            // First get a rgb Color from the old color number
            Color rgbColor;
            if (oldColorDepth == GameColorDepth.Palette)
            {
                rgbColor = palette[colourNumber].Colour;
            }
            else
            {
                rgbColor = AgsColourNumberToColorDirect(colourNumber);
            }

            // Then generate a new color number for the new depth setting
            if (gameColorDepth == GameColorDepth.Palette)
            {
                return FindNearestColourInGamePalette(rgbColor, palette);
            }
            else
            {
                return ColorToAgsColourNumberDirect(rgbColor);
            }
        }

        /// <summary>
        /// (LEGACY) Lookup table for scaling 5 bit colors up to 8 bits,
        /// copied from Allegro 4 library in order to match Editor and Engine.
        /// </summary>
        private readonly static byte[] RGBScale5 = new byte[32]
        {
           0,   8,   16,  24,  33,  41,  49,  57,
           66,  74,  82,  90,  99,  107, 115, 123,
           132, 140, 148, 156, 165, 173, 181, 189,
           198, 206, 214, 222, 231, 239, 247, 255
        };

        /// <summary>
        /// (LEGACY) Lookup table for scaling 6 bit colors up to 8 bits,
        /// copied from Allegro 4 library in order to match Editor and Engine.
        /// </summary>
        private readonly static byte[] RGBScale6 = new byte[64]
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

        /// <summary>
        /// Generates a new colour number value from a legacy number.
        /// </summary>
        public static int RemapFromLegacyColourNumber(int legacyColourNumber, PaletteEntry[] palette, GameColorDepth gameColorDepth, bool isBackgroundColor)
        {
            // For 8-bit games simply treat the color number as a palette index
            if (gameColorDepth == GameColorDepth.Palette)
            {
                return legacyColourNumber;
            }

            // Special color number 0 is treated depending on its purpose:
            // * background color becomes fully transparent;
            // * foreground color becomes opaque black
            if (legacyColourNumber == 0)
            {
                return isBackgroundColor ? 0 : (0 | (0xFF << 24));
            }

            // Special color numbers 1-31 were always interpreted as palette indexes;
            // for them we compose a 32-bit ARGB from the palette entry
            if ((legacyColourNumber > 0) && (legacyColourNumber < 32))
            {
                var rgbColor = palette[legacyColourNumber].Colour;
                return rgbColor.B | (rgbColor.G << 8) | (rgbColor.R << 16) | (0xFF << 24); // always opaque
            }

            // The rest is a R5G6B5 color; we convert it to a proper 32-bit ARGB;
            // color is always opaque when ported from legacy projects
            byte red = RGBScale5[(legacyColourNumber >> 11) & 0x1f];
            byte green = RGBScale6[(legacyColourNumber >> 5) & 0x3f];
            byte blue = RGBScale5[(legacyColourNumber) & 0x1f];
            return blue | (green << 8) | (red << 16) | (0xFF << 24);
        }

        /// <summary>
        /// Generates a legacy (AGS 3.*) color number from a Color.
        /// </summary>
        public static int MapRgbColorToLegacyColourNumber(Color rgbColor, PaletteEntry[] palette, GameColorDepth gameColorDepth)
        {
            int green = rgbColor.G;

            if (rgbColor.R == 0 && rgbColor.G == 0 && rgbColor.B > 0)
            {
                // make sure the colour number doesn't end up being a special EGA colour
                green = 4;
            }
            else if (gameColorDepth == GameColorDepth.Palette)
            {
                return FindNearestColourInGamePalette(rgbColor, palette);
            }

            // Generate a 16-bit R5G6R5 color number
            return (rgbColor.B >> 3) + ((rgbColor.G >> 2) << 5) + ((rgbColor.R >> 3) << 11);
        }

        /// <summary>
        /// Makes a opaque colour number value from a number which presumably may not have an alpha component.
        /// </summary>
        public static int MakeOpaque(int colorNumber, GameColorDepth gameColorDepth)
        {
            // For 8-bit games simply treat the color number as a palette index
            if (gameColorDepth == GameColorDepth.Palette)
            {
                return colorNumber;
            }

            return colorNumber | (0xFF << 24);
        }
    }
}
