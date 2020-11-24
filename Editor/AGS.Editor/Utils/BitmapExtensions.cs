using AGS.Types;
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace AGS.Editor
{
    public static class BitmapExtensions
    {
        /// <summary>
        /// Gives a scaled deep copy of the input image.
        /// </summary>
        /// <param name="bmp">The image to copy and scale.</param>
        /// <param name="width">Scale the image to this width.</param>
        /// <param name="height">Scale the image to this height.</param>
        /// <returns>A scaled deep copy of the input image.</returns>
        public static Bitmap Scale(this Bitmap bmp, int width, int height) => bmp.Mutate(width, height, g =>
        {
            g.DrawImage(bmp, 0, 0, width, height);
        });

        /// <summary>
        /// Gives back a deep copy of the bitmap with a drawn line.
        /// </summary>
        /// <param name="bmp">The bitmap we want to copy and draw on.</param>
        /// <param name="p1">The starting point for the line.</param>
        /// <param name="p2">The end point for the line.</param>
        /// <param name="color">The color of the line.</param>
        /// <param name="scale">Adjust coordinates for the input scale.</param>
        /// <returns>A new bitmap with a line drawn on it.</returns>
        public static Bitmap DrawLine(this Bitmap bmp, Point p1, Point p2, Color color, double scale = 1.0) => bmp.Mutate(g =>
        {
            p1 = new Point((int)(p1.X * scale), (int)(p1.Y * scale));
            p2 = new Point((int)(p2.X * scale), (int)(p2.Y * scale));

            g.DrawImage(bmp, 0, 0);
            g.DrawLine(new Pen(color), p1, p2);
        });

        /// <summary>
        /// Makes a deep copy of the bitmap that we can perform drawing operations on, and preserve the pixel format.
        /// </summary>
        /// <param name="bmp">The bitmap to deep copy.</param>
        /// <param name="mutate">The drawable <see cref="Graphics"/> in a lambda.</param>
        /// <returns>A deep copy of the bitmap with the executed drawing operations.</returns>
        public static Bitmap Mutate(this Bitmap bmp, Action<Graphics> mutate) => bmp.Mutate(bmp.Width, bmp.Height, mutate);

        /// <summary>
        /// Makes a deep copy of the bitmap that we can perform drawing operations on, and preserve the pixel format.
        /// </summary>
        /// <param name="bmp">The bitmap to deep copy.</param>
        /// <param name="width">The width of the resulting bitmap.</param>
        /// <param name="height">The height of the resulting bitmap.</param>
        /// <param name="mutate">The drawable <see cref="Graphics"/> in a lambda.</param>
        /// <returns>A deep copy of the bitmap with the executed drawing operations.</returns>
        public static Bitmap Mutate(this Bitmap bmp, int width, int height, Action<Graphics> mutate)
        {
            if (width <= 0) throw new ArgumentException("Scale factor must be greater than 0.", nameof(width));
            if (height <= 0) throw new ArgumentException("Scale factor must be greater than 0.", nameof(height));

            // Drawing operations is not supported on 8-bit images so we have to create a default 32-bit image buffer for drawing
            using (Bitmap drawBuffer = new Bitmap(width, height))
            {
                using (Graphics graphics = Graphics.FromImage(drawBuffer))
                {
                    graphics.CompositingMode = CompositingMode.SourceCopy;
                    graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
                    mutate(graphics);
                }

                Bitmap result = new Bitmap(width, height, bmp.PixelFormat);

                // Make sure we use the correct palette for indexed images
                if (bmp.PixelFormat == PixelFormat.Format8bppIndexed)
                {
                    result.SetPaletteFromGlobalPalette();
                    drawBuffer.Palette = result.Palette;
                }

                // Write drawing buffer to the result image and retain the pixel format.
                result.SetRawData(drawBuffer.GetRawData(bmp.PixelFormat), bmp.PixelFormat);
                return result;
            }
        }

        /// <summary>
        /// Gets the raw data from the bitmap as a byte array.
        /// </summary>
        /// <param name="bmp">The image to get the raw data from.</param>
        /// <returns>The raw data as a byte array.</returns>
        public static byte[] GetRawData(this Bitmap bmp) => bmp.GetRawData(bmp.PixelFormat);

        /// <summary>
        /// Gets the raw data from the bitmap as a byte array.
        /// </summary>
        /// <param name="bmp">The image to get the raw data from.</param>
        /// <param name="pixelFormat">The pixel format to read the raw data as.</param>
        /// <returns>The raw data as a byte array.</returns>
        public static byte[] GetRawData(this Bitmap bmp, PixelFormat pixelFormat)
        {
            BitmapData data = bmp.LockBits(new Rectangle(0, 0, bmp.Width, bmp.Height), ImageLockMode.ReadOnly, pixelFormat);
            int pixelCount = Math.Abs(data.Stride) * data.Height;
            byte[] bitmapRawData = new byte[pixelCount];
            Marshal.Copy(data.Scan0, bitmapRawData, 0, pixelCount);
            bmp.UnlockBits(data);

            return bitmapRawData;
        }

        /// <summary>
        /// Sets the raw data from the byte array to the bitmap.
        /// </summary>
        /// <param name="bmp">The bitmap to set the raw data to.</param>
        /// <param name="rawData">The raw data to set into the bitmap.</param>
        /// <param name="pixelFormat">The pixel format to read the raw data as.</param>
        public static void SetRawData(this Bitmap bmp, byte[] rawData, PixelFormat pixelFormat)
        {
            BitmapData data = bmp.LockBits(new Rectangle(0, 0, bmp.Width, bmp.Height), ImageLockMode.WriteOnly, pixelFormat);
            int pixelCount = Math.Abs(data.Stride) * data.Height;
            Marshal.Copy(rawData, 0, data.Scan0, pixelCount);
            bmp.UnlockBits(data);
        }

        /// <summary>
        /// Sets the raw data from the byte array to the bitmap.
        /// </summary>
        /// <param name="bmp">The bitmap to set the raw data to.</param>
        /// <param name="rawData">The raw data to set into the bitmap.</param>
        public static void SetRawData(this Bitmap bmp, byte[] rawData) => bmp.SetRawData(rawData, bmp.PixelFormat);

        /// <summary>
        /// Sets the current game's global palette to a bitmap.
        /// </summary>
        /// <param name="bmp">The bitmap to set to global palette to.</param>
        public static void SetPaletteFromGlobalPalette(this Bitmap bmp)
        {
            ColorPalette palette = bmp.Palette;

            foreach (PaletteEntry global in Factory.AGSEditor.CurrentGame.Palette)
            {
                palette.Entries[global.Index] = global.Colour;
            }

            bmp.Palette = palette; // Get Bitmap.Palette is deep copy so we need to set it back
        }
    }
}
