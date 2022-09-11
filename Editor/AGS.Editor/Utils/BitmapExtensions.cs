using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace AGS.Editor
{
    public static class BitmapTransparencyFixUp
    {
        // methods from https://stackoverflow.com/questions/4803935/free-file-locked-by-new-bitmapfilepath/48170549#48170549
        // and from https://stackoverflow.com/questions/24074641/how-to-read-8-bit-png-image-as-8-bit-png-image-only

        /// <summary>
        /// Clones an image object to free it from any backing resources.
        /// Code taken from http://stackoverflow.com/a/3661892/ with some extra fixes.
        /// </summary>
        /// <param name="sourceImage">The image to clone</param>
        /// <returns>The cloned image</returns>
        public static Bitmap CloneImage(Bitmap sourceImage)
        {
            Rectangle rect = new Rectangle(0, 0, sourceImage.Width, sourceImage.Height);
            Bitmap targetImage = new Bitmap(rect.Width, rect.Height, sourceImage.PixelFormat);
            targetImage.SetResolution(sourceImage.HorizontalResolution, sourceImage.VerticalResolution);
            BitmapData sourceData = sourceImage.LockBits(rect, ImageLockMode.ReadOnly, sourceImage.PixelFormat);
            BitmapData targetData = targetImage.LockBits(rect, ImageLockMode.WriteOnly, targetImage.PixelFormat);
            Int32 actualDataWidth = ((Image.GetPixelFormatSize(sourceImage.PixelFormat) * rect.Width) + 7) / 8;
            Int32 h = sourceImage.Height;
            Int32 origStride = sourceData.Stride;
            Boolean isFlipped = origStride < 0;
            origStride = Math.Abs(origStride); // Fix for negative stride in BMP format.
            Int32 targetStride = targetData.Stride;
            Byte[] imageData = new Byte[actualDataWidth];
            IntPtr sourcePos = sourceData.Scan0;
            IntPtr destPos = targetData.Scan0;
            // Copy line by line, skipping by stride but copying actual data width
            for (Int32 y = 0; y < h; y++)
            {
                Marshal.Copy(sourcePos, imageData, 0, actualDataWidth);
                Marshal.Copy(imageData, 0, destPos, actualDataWidth);
                sourcePos = new IntPtr(sourcePos.ToInt64() + origStride);
                destPos = new IntPtr(destPos.ToInt64() + targetStride);
            }
            targetImage.UnlockBits(targetData);
            sourceImage.UnlockBits(sourceData);
            // Fix for negative stride on BMP format.
            if (isFlipped)
                targetImage.RotateFlip(RotateFlipType.Rotate180FlipX);
            // For indexed images, restore the palette. This is not linking to a referenced
            // object in the original image; the getter of Palette creates a new object when called.
            if ((sourceImage.PixelFormat & PixelFormat.Indexed) != 0)
                targetImage.Palette = sourceImage.Palette;
            // Restore DPI settings
            targetImage.SetResolution(sourceImage.HorizontalResolution, sourceImage.VerticalResolution);
            return targetImage;
        }

        private static Byte[] PNG_IDENTIFIER = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

        /// <summary>
        /// Loads an image, checks if it is a PNG containing palette transparency, and if so, ensures it loads correctly.
        /// The theory can be found at http://www.libpng.org/pub/png/book/chapter08.html
        /// Always unlocked
        /// </summary>
        /// <param name="filename">Filename to load</param>
        /// <returns>The loaded image</returns>
        public static Bitmap LoadBitmap(String filename)
        {
            Byte[] data = File.ReadAllBytes(filename);
            return LoadBitmap(data);
        }

        /// <summary>
        /// Loads an image, checks if it is a PNG containing palette transparency, and if so, ensures it loads correctly.
        /// The theory can be found at http://www.libpng.org/pub/png/book/chapter08.html
        /// </summary>
        /// <param name="data">File data to load</param>
        /// <returns>The loaded image</returns>
        public static Bitmap LoadBitmap(Byte[] data)
        {
            Byte[] transparencyData = null;
            if (data.Length > PNG_IDENTIFIER.Length)
            {
                // Check if the image is a PNG.
                Byte[] compareData = new Byte[PNG_IDENTIFIER.Length];
                Array.Copy(data, compareData, PNG_IDENTIFIER.Length);
                if (PNG_IDENTIFIER.SequenceEqual(compareData))
                {
                    // Check if it contains a palette.
                    Int32 plteOffset = FindChunk(data, "PLTE");
                    if (plteOffset != -1)
                    {
                        // Check if it contains a palette transparency chunk.
                        Int32 trnsOffset = FindChunk(data, "tRNS");
                        if (trnsOffset != -1)
                        {
                            // Get chunk
                            Int32 trnsLength = GetChunkDataLength(data, trnsOffset);
                            transparencyData = new Byte[trnsLength];
                            Array.Copy(data, trnsOffset + 8, transparencyData, 0, trnsLength);
                            // filter out the palette alpha chunk, make new data array
                            Byte[] data2 = new Byte[data.Length - (trnsLength + 12)];
                            Array.Copy(data, 0, data2, 0, trnsOffset);
                            Int32 trnsEnd = trnsOffset + trnsLength + 12;
                            Array.Copy(data, trnsEnd, data2, trnsOffset, data.Length - trnsEnd);
                            data = data2;
                        }
                    }
                }
            }
            Bitmap loadedImage;
            using (MemoryStream ms = new MemoryStream(data))
            using (Bitmap tmp = new Bitmap(ms))
                loadedImage = CloneImage(tmp);
            ColorPalette pal = loadedImage.Palette;
            if (pal.Entries.Length == 0 || transparencyData == null)
                return loadedImage;
            for (Int32 i = 0; i < pal.Entries.Length; i++)
            {
                if (i >= transparencyData.Length)
                    break;
                Color col = pal.Entries[i];
                pal.Entries[i] = Color.FromArgb(transparencyData[i], col.R, col.G, col.B);
            }
            loadedImage.Palette = pal;
            return loadedImage;
        }

        /// <summary>
        /// Finds the start of a png chunk. This assumes the image is already identified as PNG.
        /// It does not go over the first 8 bytes, but starts at the start of the header chunk.
        /// </summary>
        /// <param name="data">The bytes of the png image</param>
        /// <param name="chunkName">The name of the chunk to find.</param>
        /// <returns>The index of the start of the png chunk, or -1 if the chunk was not found.</returns>
        private static Int32 FindChunk(Byte[] data, String chunkName)
        {
            if (chunkName.Length != 4)
                throw new ArgumentException("Chunk must be 4 characters!", "chunkName");
            Byte[] chunkNamebytes = Encoding.ASCII.GetBytes(chunkName);
            if (chunkNamebytes.Length != 4)
                throw new ArgumentException("Chunk must be 4 characters!", "chunkName");
            Int32 offset = PNG_IDENTIFIER.Length;
            Int32 end = data.Length;
            Byte[] testBytes = new Byte[4];
            // continue until either the end is reached, or there is not enough space behind it for reading a new chunk
            while (offset + 12 <= end)
            {
                Array.Copy(data, offset + 4, testBytes, 0, 4);
                // Alternative for more visual debugging:
                //String currentChunk = Encoding.ASCII.GetString(testBytes);
                //if (chunkName.Equals(currentChunk))
                //    return offset;
                if (chunkNamebytes.SequenceEqual(testBytes))
                    return offset;
                Int32 chunkLength = GetChunkDataLength(data, offset);
                // chunk size + chunk header + chunk checksum = 12 bytes.
                offset += 12 + chunkLength;
            }
            return -1;
        }

        private static Int32 GetChunkDataLength(Byte[] data, Int32 offset)
        {
            if (offset + 4 > data.Length)
                throw new IndexOutOfRangeException("Bad chunk size in png image.");
            // Don't want to use BitConverter; then you have to check platform endianness and all that mess.
            Int32 length = data[offset + 3] + (data[offset + 2] << 8) + (data[offset + 1] << 16) + (data[offset] << 24);
            if (length < 0)
                throw new IndexOutOfRangeException("Bad chunk size in png image.");
            return length;
        }
    }

    public static class BitmapExtensions
    {
        /// <summary>
        /// Loads a <see cref="Bitmap"/> from file that doesn't lock the original file.
        /// </summary>
        public static Bitmap LoadNonLockedBitmap(string path)
        {
            return BitmapTransparencyFixUp.LoadBitmap(path);
        }

        /// <summary>
        /// Gets an integer with the color depth of the image.
        /// </summary>
        /// <param name="bmp">The image to get the color depth from.</param>
        /// <returns>An integer with the color depth.</returns>
        public static int GetColorDepth(this Bitmap bmp) => Image.GetPixelFormatSize(bmp.PixelFormat);

        /// <summary>
        /// Gives a scaled deep copy of the input image.
        /// </summary>
        /// <param name="bmp">The image to copy and scale.</param>
        /// <param name="width">Scale the image to this width.</param>
        /// <param name="height">Scale the image to this height.</param>
        /// <returns>A scaled deep copy of the input image.</returns>
        public static Bitmap ScaleIndexed(this Bitmap bmp, int width, int height)
        {
            if (!bmp.IsIndexed()) throw new ArgumentException($"{nameof(bmp)} must be a indexed bitmap.");
            if (width <= 0) throw new ArgumentException("Scale factor must be greater than 0.", nameof(width));
            if (height <= 0) throw new ArgumentException("Scale factor must be greater than 0.", nameof(height));

            Bitmap res = new Bitmap(width, height, bmp.PixelFormat) { Palette = bmp.Palette };
            res.SetPaletteFromGlobalPalette();
            int bmpRowPaddedWidth = (int)Math.Floor((bmp.GetColorDepth() * bmp.Width + 31.0) / 32.0) * 4;
            int resRowPaddedWidth = (int)Math.Floor((res.GetColorDepth() * res.Width + 31.0) / 32.0) * 4;
            byte[] resultRawData = new byte[resRowPaddedWidth * height];
            byte[] bmpRawData = bmp.GetRawData();

            // Nearest Neighbor Interpolation
            double ratioWidth = (double)bmp.Width / width;
            double ratioHeight = (double)bmp.Height / height;
            int[] rowMap =
                Enumerable.Range(0, width).Select(i => (int)Math.Floor((double)(i * ratioWidth))).ToArray();
            int[] columnMap =
                Enumerable.Range(0, height).Select(i => (int)Math.Floor((double)(i * ratioHeight))).ToArray();

            for (int y = 0; y < height; y++)
                for (int x = 0; x < resRowPaddedWidth; x++)
                    resultRawData[(resRowPaddedWidth * y) + x] = x < width
                        ? bmpRawData[(bmpRowPaddedWidth * columnMap[y]) + rowMap[x]]
                        : (byte)0; // Padded area 0

            res.SetRawData(resultRawData);
            return res;
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

        public static void SetGlobalPaletteFromPalette(this Bitmap bmp)
        {
            PaletteEntry[] palettes = Factory.AGSEditor.CurrentGame.Palette;
            foreach (PaletteEntry global in palettes.Where(p => p.ColourType == PaletteColourType.Background))
            {
                palettes[global.Index].Colour = bmp.Palette.Entries[global.Index];
            }
        }

        /// <summary>
        /// Sets the current game's global palette to a bitmap.
        /// </summary>
        /// <param name="bmp">The bitmap to set to global palette to.</param>
        public static void SetPaletteFromGlobalPalette(this Bitmap bmp)
        {
            ColorPalette palette = bmp.Palette;

            foreach (PaletteEntry global in Factory.AGSEditor.CurrentGame.Palette)
            {
                palette.Entries[global.Index] = Color.FromArgb(255, global.Colour);
            }

            bmp.Palette = palette; // Get Bitmap.Palette is deep copy so we need to set it back
        }

        /// <summary>
        /// Check if <see cref="Bitmap"/> is indexed image.
        /// </summary>
        /// <param name="bmp">Check if this argument is indexed image.</param>
        /// <returns>A bool indicating if this image is indexed.</returns>
        public static bool IsIndexed(this Bitmap bmp) =>
            bmp.PixelFormat == PixelFormat.Format1bppIndexed ||
            bmp.PixelFormat == PixelFormat.Format4bppIndexed ||
            bmp.PixelFormat == PixelFormat.Format8bppIndexed;

        /// <summary>
        /// Checks if the x and y coordinate is within the image.
        /// </summary>
        /// <param name="position">The position to check against.</param>
        /// <returns>True if the position is inside the image. False if outside.</returns>
        public static bool Intersects(this Bitmap bmp, Point position)
        {
            return position.X >= 0 && position.X < bmp.Width && position.Y >= 0 && position.Y < bmp.Height;
        }
    }

    /// <summary>
    /// A graphics API similar to <see cref="Graphics"/> that supports indexed images. Drawing operations
    /// are not written back to the image until Dispose is invoked.
    /// </summary>
    public class IndexedGraphics : IDisposable
    {
        private readonly Bitmap _bmp;
        private readonly byte[] _pixels;
        private readonly int _paddedWidth; // Bitmap data width is rounded up to a multiple of 4

        private IndexedGraphics(Bitmap bmp)
        {
            if (bmp == null) throw new ArgumentNullException(nameof(bmp));
            if (!bmp.IsIndexed()) throw new ArgumentException(
                $"{nameof(bmp)} must be a indexed bitmap. Use {nameof(Graphics)} instead for non-indexed images.");

            _bmp = bmp;
            _pixels = _bmp.GetRawData();
            _paddedWidth = (int)Math.Floor((_bmp.GetColorDepth() * _bmp.Width + 31.0) / 32.0) * 4;
        }

        public static IndexedGraphics FromBitmap(Bitmap bmp) => new IndexedGraphics(bmp);

        public void Dispose()
        {
            _bmp.SetRawData(_pixels);
        }

        /// <summary>
        /// Draws a line.
        /// </summary>
        /// <param name="color">The color index of the line.</param>
        /// <param name="p0">The starting point for the line.</param>
        /// <param name="p1">The end point for the line.</param>
        /// <param name="scale">Adjust coordinates for the input scale.</param>
        public void DrawLine(int color, Point p0, Point p1, double scale = 1.0)
        {
            p0 = new Point((int)(p0.X * scale), (int)(p0.Y * scale));
            p1 = new Point((int)(p1.X * scale), (int)(p1.Y * scale));

            IEnumerable<Point> pixels;
            if (Math.Abs(p1.Y - p0.Y) < Math.Abs(p1.X - p0.X))
                pixels = p0.X > p1.X
                    ? BresenhamsLineLow(p1, p0)
                    : BresenhamsLineLow(p0, p1);
            else
                pixels = p0.Y > p1.Y
                    ? BresenhamsLineHigh(p1, p0)
                    : BresenhamsLineHigh(p0, p1);

            byte colorAsByte = (byte)color;

            foreach (int i in pixels.Where(p => _bmp.Intersects(p)).Select(p => (_paddedWidth * p.Y) + p.X))
                _pixels[i] = colorAsByte;
        }

        /// <summary>
        /// Draws a rectangle.
        /// </summary>
        /// <param name="color">The color index of the line.</param>
        /// <param name="p0">The starting point for the line.</param>
        /// <param name="p1">The end point for the line.</param>
        /// <param name="scale">Adjust coordinates for the input scale.</param>
        public void FillRectangle(int color, Point p0, Point p1, double scale = 1.0)
        {
            Point origin = new Point(Math.Min(p0.X, p1.X), Math.Min(p0.Y, p1.Y));
            Point originScaled = new Point((int)(origin.X * scale), (int)(origin.Y * scale));

            Size size = new Size(Math.Abs(p0.X - p1.X), Math.Abs(p0.Y - p1.Y));
            Size sizeScaled = new Size((int)(size.Width * scale), (int)(size.Height * scale));

            byte colorAsByte = (byte)color;
            IEnumerable<Point> pixels = CalculatRecanglePixels(originScaled, sizeScaled);

            foreach (int i in pixels.Where(p => _bmp.Intersects(p)).Select(p => (_paddedWidth * p.Y) + p.X))
                _pixels[i] = colorAsByte;
        }

        /// <summary>
        /// Fills the area with the target color
        /// </summary>
        /// <param name="color">The color to use for the filling.</param>
        /// <param name="position">The position we want to fill from.</param>
        /// <param name="scale">Adjust coordinates for the input scale.</param>
        /// <returns>A new bitmap with the area filled.</returns>
        public void FillArea(int color, Point position, double scale)
        {
            Point positionScaled = new Point((int)(position.X * scale), (int)(position.Y * scale));
            FloodFillImage(_pixels, positionScaled, _bmp.Size, _pixels[(positionScaled.Y * _bmp.Width) + positionScaled.X], (byte)color);
        }

        private static IEnumerable<Point> CalculatRecanglePixels(Point origin, Size size)
        {
            for (int y = origin.Y; y <= origin.Y + size.Height; y++)
                for (int x = origin.X; x <= origin.X + size.Width; x++)
                    yield return new Point(x, y);
        }

        private static IEnumerable<Point> BresenhamsLineLow(Point p0, Point p1)
        {
            Point delta = new Point(p1.X - p0.X, p1.Y - p0.Y);
            int yIncrement = 1;

            if (delta.Y < 0)
            {
                yIncrement = -1;
                delta.Y = -delta.Y;
            }

            int difference = (2 * delta.Y) - delta.X;

            for (Point p = p0; p.X <= p1.X; p.X++)
            {
                yield return p;

                if (difference >= 0)
                {
                    p.Y += yIncrement;
                    difference += 2 * (delta.Y - delta.X);
                }
                else
                    difference += 2 * delta.Y;
            }
        }

        private static IEnumerable<Point> BresenhamsLineHigh(Point p0, Point p1)
        {
            Point delta = new Point(p1.X - p0.X, p1.Y - p0.Y);
            int xIncrement = 1;

            if (delta.X < 0)
            {
                xIncrement = -1;
                delta.X = -delta.X;
            }

            int difference = (2 * delta.X) - delta.Y;

            for (Point p = p0; p.Y <= p1.Y; p.Y++)
            {
                yield return p;

                if (difference >= 0)
                {
                    p.X += xIncrement;
                    difference += 2 * (delta.X - delta.Y);
                }
                else
                    difference += 2 * delta.X;
            }
        }

        /// <summary>
        /// Flood fill algorithm with for-loop instead of recursion to avoid stack overflow issues.
        /// </summary>
        /// <param name="image">The raw byte array of the image.</param>
        /// <param name="position">The starting position for the fill.</param>
        /// <param name="size">The dimensions of the image.</param>
        /// <param name="initial">The color of the starting position.</param>
        /// <param name="replacement">The color to replace the pixels with.</param>
        private static void FloodFillImage(byte[] image, Point position, Size size, byte initial, byte replacement)
        {
            Queue<Point> queue = new Queue<Point>();
            queue.Enqueue(position);

            while (queue.Any())
            {
                position = queue.Dequeue();

                if (position.X >= 0 && position.X < size.Width && position.Y >= 0 && position.Y < size.Height)
                {
                    int i = (position.Y * size.Width) + position.X;

                    if (image[i] == initial && image[i] != replacement)
                    {
                        image[i] = replacement;

                        queue.Enqueue(new Point(position.X + 1, position.Y));
                        queue.Enqueue(new Point(position.X - 1, position.Y));
                        queue.Enqueue(new Point(position.X, position.Y + 1));
                        queue.Enqueue(new Point(position.X, position.Y - 1));
                    }
                }
            }
        }
    }
}
