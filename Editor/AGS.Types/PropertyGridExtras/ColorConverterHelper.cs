using System;
using System.Drawing;

namespace AGS.Types
{
    internal static class ColorConverterHelper
    {
        public static GameColorDepth ColorMode = GameColorDepth.TrueColor;
        public const int SpecialColorSlotCount = 32;

        public static Color ColorFromString(string value)
        {
            value = value.Trim();
            if (string.IsNullOrEmpty(value))
                return Color.Black; // return black color on failure

            // First check explicit notations
            if (value.StartsWith("#"))
            {
                return Utilities.ColorFromHTMLHex(value);
            }
            else if (value.StartsWith("0x"))
            {
                return Utilities.ColorFromARGBHex(value);
            }

            // Try parse one as a traditional AGS color number (decimal)
            int colorNumber;
            if (int.TryParse(value, out colorNumber))
                return AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(colorNumber);

            // If it's not a single number of any kind, then try to parse as color components
            // separated either by comma or semicolon
            Color color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ',');
            if (color.IsEmpty)
                color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ';');
            if (!color.IsEmpty)
                return color;

            return Color.Black; // return black color on failure
        }

        public static string ColorToString(Color color)
        {
            // In AGS v3.* there's no "alpha" support in color properties.
            return string.Format($"{color.R}; {color.G}; {color.B}");
        }

        public static int AgsColorNumberFromString(string value)
        {
            // Special slots must be displayed as indexes in any case
            int colorNumber = 0;
            bool parsedAsNumber = int.TryParse(value, out colorNumber);
            if ((ColorMode == GameColorDepth.Palette) || (parsedAsNumber && (colorNumber < SpecialColorSlotCount)))
            {
                return colorNumber;
            }

            Color color = ColorFromString(value).ToRGB();
            return AGSColor.ColorMapper.MapRgbColorToAgsColourNumber(color);
        }

        public static int AgsColorNumberFromRGBString(string value)
        {
            Color color = ColorFromString(value).ToRGB();
            return AGSColor.ColorMapper.MapRgbColorToAgsColourNumber(color);
        }

        public static string AgsColorNumberToString(int value)
        {
            // Special slots must be displayed as indexes in any case
            if ((ColorMode == GameColorDepth.Palette) || (value < SpecialColorSlotCount))
                return value.ToString();
            return ColorToString(AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(value));
        }

        public static string AgsColorNumberToRGBString(int value)
        {
            return ColorToString(AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(value));
        }
    }
}
