using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// PaletteUtilities is meant to be a group of methods which handle AGS Game Palette,
    /// and perform various operations over it, or between AGS Palette and Image's Palette.
    /// </summary>
    public static class PaletteUtilities
    {
        static PaletteUtilities()
        {
            BestFitInit();
        }

        /// <summary>
        /// Assigns Background slots in the AGS palette from this Bitmap.
        /// </summary>
        public static void CopyToAGSBackgroundPalette(this Bitmap bmp, PaletteEntry[] dstPalette)
        {
            var srcPalette = bmp.Palette;
            foreach (PaletteEntry global in dstPalette.Where(
                p => (p.Index < srcPalette.Entries.Length) && (p.ColourType == PaletteColourType.Background)))
            {
                dstPalette[global.Index].Colour = srcPalette.Entries[global.Index];
            }
        }

        /// <summary>
        /// Assigns the AGS palette to a bitmap.
        /// Note that both Game-wide and Background palette slots are copied.
        /// This assumes that the game palette has been prepared beforehand, by merging
        /// game-wide and room palette colors.
        /// </summary>
        /// <param name="bmp">The bitmap to set to global palette to.</param>
        public static void SetFromAGSPalette(this Bitmap bmp, PaletteEntry[] srcPalette)
        {
            ColorPalette palette = bmp.Palette;

            foreach (PaletteEntry global in srcPalette)
            {
                palette.Entries[global.Index] = Color.FromArgb(255, global.Colour);
            }

            bmp.Palette = palette; // Get Bitmap.Palette is deep copy so we need to set it back
        }

        public static void RemapBackground(Bitmap scene, bool exactPal, PaletteEntry[] gamePalette, out int colorsImage, out int colorsLimit)
        {
            if (scene.Palette.Entries.Length == 0)
            {
                colorsImage = 0;
                colorsLimit = 0;
                return;
            }

            ColorPalette palette = scene.Palette;
            /*
            // Unconditionally copy any Locked colors to the bitmap palette
            foreach (var color in gamePalette.Where(p => p.ColourType == PaletteColourType.Locked))
            {
                palette.Entries[color.Index] = color.Colour;
            }
            */

            // Exact palette: keep any color of Background indexes as-is
            if (exactPal)
            {
                // We should still copy all the Game-wide slots over,
                // otherwise the user will get false picture in the Editor.
                foreach (var color in gamePalette.Where(
                    p => (p.Index < palette.Entries.Length) && (p.ColourType == PaletteColourType.Gamewide)))
                {
                    palette.Entries[color.Index] = color.Colour;
                }
                scene.Palette = palette;
                colorsImage = 0; // just reset them, no use in this case
                colorsLimit = 0;
                return;
            }

            // Now do palette remap...
            // Find how many slots there are reserved for backgrounds
            // NOTE: this was counting non-Gamewide in original code, meaning this counts Bg + Locked
            int bgSlotCount = gamePalette.Count(p => p.ColourType != PaletteColourType.Gamewide);
            // Find which colours from the image palette are actually used
            var pixels = scene.GetRawData();
            int[] slotUsed = new int[256];
            int usedSlotCount = 0;
            for (int p = 0; p < pixels.Length; ++p)
            {
                slotUsed[pixels[p]]++;
            }
            for (int i = 0; i < 256; ++i)
            {
                if (slotUsed[i] > 0)
                    usedSlotCount++;
            }

            Color[] srcColors = new Color[256];
            for (int i = 0; i < palette.Entries.Length; ++i)
            {
                srcColors[i] = Color.FromArgb(0xFF, palette.Entries[i].R, palette.Entries[i].G, palette.Entries[i].B);
            }
            // Fill any remaining slots with black color
            for (int i = palette.Entries.Length; i < 256; ++i)
            {
                srcColors[i] = Color.FromArgb(0xFF, 0, 0, 0);
            }

            // Count and remember the unique colours in the image
            // We have to do this separately, because some of the palette colors could be duplicates.
            Color[] uniqueColors = new Color[256];
            int uniqueColorCount = 0;
            for (int i = 0; i < 256; ++i)
            {
                // slot not used
                if (slotUsed[i] == 0)
                {
                    continue;
                }

                // full-black color, skip it
                if ((srcColors[i].R == 0) & (srcColors[i].G == 0) & (srcColors[i].B == 0))
                {
                    continue;
                }

                bool foundRepeat = false;
                for (int j = 0; j < uniqueColorCount; ++j)
                {
                    if ((srcColors[i].R == uniqueColors[j].R) &
                        (srcColors[i].G == uniqueColors[j].G) &
                        (srcColors[i].B == uniqueColors[j].B))
                    {
                        foundRepeat = true;
                        break;
                    }
                }

                if (foundRepeat)
                {
                    continue;
                }

                // Save new unique color
                uniqueColors[uniqueColorCount++] = srcColors[i];
            }

            // Create new set of colors for the final mapping
            Color[] finalColors = new Color[256];
            // Clear the game-wide color slots, so that they are not used during following remap
            foreach (var color in gamePalette.Where(p => p.ColourType == PaletteColourType.Gamewide))
            {
                finalColors[color.Index] = Color.Black;
            }
            // Fill the background slots in the palette with the unique colours
            // Start from end of palette and work backwards
            for (int uc = 0, slot = 255; (uc < uniqueColorCount) && (slot >= 0); ++uc, --slot)
            {
                // find the next background slot for this unique color
                for (; (slot >= 0) &&
                    (gamePalette[slot].ColourType != PaletteColourType.Background);
                    --slot) ;

                if (slot >= 0)
                {
                    finalColors[slot] = uniqueColors[uc];
                }
            }

            /*
            // Copy any Locked colors over the final palette
            foreach (var color in gamePalette.Where(p => p.ColourType == PaletteColourType.Locked))
            {
                finalColors[color.Index] = color.Colour;
            }
            */

            // Use the final color list to remap the palette
            RemapPixels(pixels, srcColors, finalColors, false /* don't keep transparency */);

            // Assign resulting pixel array and new palette to the bitmap;
            // Un-nativize RGB from 64-unit format
            ColorPalette finalPalette = BitmapExtensions.CreateColorPalette();
            for (int i = 0; i < 256; ++i)
            {
                finalPalette.Entries[i] = Color.FromArgb(0xFF, finalColors[i].R, finalColors[i].G, finalColors[i].B);
            }
            scene.SetRawData(pixels);
            scene.Palette = finalPalette;
            colorsImage = uniqueColorCount;
            colorsLimit = bgSlotCount;
        }

        /// <summary>
        /// Remaps pixel values from old palette to the new palette,
        /// by finding the "best match" colors in the new palette.
        /// </summary>
        private static void RemapPixels(byte[] pixels, Color[] oldPal, Color[] newPal, bool keepTransparency)
        {
            byte[] colorMap = new byte[256];
            for (int i = 0; i < 256; ++i)
            {
                if ((oldPal[i].R == 0) && (oldPal[i].G == 0) && (oldPal[i].B == 0))
                {
                    colorMap[i] = 0;
                }
                else
                {
                    colorMap[i] = BestFitColor(newPal, oldPal[i].R, oldPal[i].G, oldPal[i].B);
                }
            }

            if (keepTransparency)
            {
                colorMap[0] = 0;
                // Any other pixels which are being mapped to 0, map to 16 instead
                // TODO: find out why "16" (was in the old code) and comment here;
                // probably a "magic" color value...
                for (int i = 1; i < 256; ++i)
                {
                    if (colorMap[i] == 0)
                        colorMap[i] = 16;
                }
            }

            for (int p = 0; p < pixels.Length; ++p)
            {
                pixels[p] = colorMap[pixels[p]];
            }
        }

        // 6k lookup table for color matching
        private static uint[] ColDiffTable = new uint[3 * 512];

        /// <summary>
        /// Color matching is done with weighted squares, which are much faster
        /// if we pregenerate a little lookup table...
        /// </summary>
        private static void BestFitInit()
        {
            for (uint i = 1; i < 256; i++)
            {
                uint k = i * i;
                ColDiffTable[0 + i] = ColDiffTable[0 + 512 - i] = k * (14 * 14);
                ColDiffTable[512 + i] = ColDiffTable[512 + 512 - i] = k * (8 * 8);
                ColDiffTable[1024 + i] = ColDiffTable[1024 + 512 - i] = k * (3 * 3);
            }
        }

        /// <summary>
        /// Searches a palette for the color closest to the requested R, G, B value.
        /// </summary>
        private static byte BestFitColor(Color[] pal, int r, int g, int b)
        {
            // only the transparent (pink) color can be mapped to index 0
            uint slot;
            if ((r == 255) && (g == 0) && (b == 255))
                slot = 0;
            else
                slot = 1;

            uint bestfit = 0;
            uint lowest = uint.MaxValue;

            for (; slot < 256; ++slot)
            {
                Color col = pal[slot];
                uint coldiff = ColDiffTable[0 + ((col.G - g) & 0x1FF)];
                if (coldiff < lowest)
                {
                    coldiff += ColDiffTable[512 + ((col.R - r) & 0x1FF)];
                    if (coldiff < lowest)
                    {
                        coldiff += ColDiffTable[1024 + ((col.B - b) & 0x1FF)];
                        if (coldiff < lowest)
                        {
                            bestfit = slot;
                            if (coldiff == 0)
                                return (byte)bestfit;
                            lowest = coldiff;
                        }
                    }
                }
            }

            return (byte)bestfit;
        }
    }
}
