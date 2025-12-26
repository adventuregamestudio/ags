using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Editor.Utils
{
    //
    // BitmapExtensions module is purposed for various manipulations with Bitmap
    // and Image objects: their loading, saving, transformation, etc.
    //

    /// <summary>
    /// BitmapHelper is a collection of replacements for Bitmap Cloning, Loading,
    /// and so forth, which improve standard .NET framework behavior in some cases.
    /// </summary>
    public static class BitmapHelper
    {
        // methods from https://stackoverflow.com/questions/4803935/free-file-locked-by-new-bitmapfilepath/48170549#48170549
        // and from https://stackoverflow.com/questions/24074641/how-to-read-8-bit-png-image-as-8-bit-png-image-only

        /// <summary>
        /// Clones an image object, freeing the image from any backing
        /// resources (prevents file lock).
        /// </summary>
        /// <param name="sourceImage">The image to clone</param>
        /// <returns>The cloned image</returns>
        public static Bitmap CloneImage(Bitmap sourceImage)
        {
            Rectangle rect = new Rectangle(0, 0, sourceImage.Width, sourceImage.Height);
            return CloneImage(sourceImage, rect);
        }

        /// <summary>
        /// Clones the portion of the image, freeing the image from any backing
        /// resources (prevents file lock).
        /// Code taken from http://stackoverflow.com/a/3661892/ with some extra fixes.
        /// </summary>
        /// <param name="sourceImage">The image to clone</param>
        /// <returns>The cloned image</returns>
        public static Bitmap CloneImage(Bitmap sourceImage, Rectangle sourceRect)
        {
            Rectangle destRect = new Rectangle(0, 0, sourceRect.Width, sourceRect.Height);
            Bitmap targetImage = new Bitmap(destRect.Width, destRect.Height, sourceImage.PixelFormat);
            targetImage.SetResolution(sourceImage.HorizontalResolution, sourceImage.VerticalResolution);
            BitmapData sourceData = sourceImage.LockBits(sourceRect, ImageLockMode.ReadOnly, sourceImage.PixelFormat);
            BitmapData targetData = targetImage.LockBits(destRect, ImageLockMode.WriteOnly, targetImage.PixelFormat);
            Int32 actualDataWidth = ((Image.GetPixelFormatSize(sourceImage.PixelFormat) * sourceRect.Width) + 7) / 8;
            Int32 h = sourceRect.Height;
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
        /// </summary>
        /// <param name="stream">Stream to load the image from</param>
        /// <returns>The loaded image</returns>
        public static Bitmap LoadBitmap(Stream stream)
        {
            Byte[] data = new Byte[stream.Length];
            stream.Read(data, 0, (int)stream.Length);
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

    /// <summary>
    /// BitmapExtensions is a collection of helpers for operations with bitmaps.
    /// </summary>
    public static class BitmapExtensions
    {
        /// <summary>
        /// Gets an integer with the color depth of the image.
        /// </summary>
        /// <param name="bmp">The image to get the color depth from.</param>
        /// <returns>An integer with the color depth.</returns>
        public static int GetColorDepth(this Bitmap bmp) => Image.GetPixelFormatSize(bmp.PixelFormat);

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
        /// Check if Bitmap contains a valid alpha channel.
        /// Returns positive for ARGB pixel format, and for bitmaps of Indexed
        /// format which palette contains alpha component.
        /// </summary>
        public static bool HasAlpha(this Bitmap bmp)
        {
            if (Bitmap.IsAlphaPixelFormat(bmp.PixelFormat))
                return true;

            if ((bmp.PixelFormat & PixelFormat.Indexed) != 0)
                return DoesPaletteHaveAlpha(bmp);

            return false;
        }

        /// <summary>
        /// Check if Bitmap's palette has alpha component.
        /// Fails if there's no palette.
        /// </summary>
        public static bool DoesPaletteHaveAlpha(this Bitmap bmp)
        {
            if (bmp.Palette == null)
                return false;

            // First test if there is a "alpha" or "halftone" flags present
            if ((bmp.Palette.Flags & (0x1 | 0x4)) == 0)
                return false;

            // Do a full scan of pal entries to see if they contain
            // any non-opaque or non-fully-transparent colors.
            for (int i = 0; i < bmp.Palette.Entries.Length; ++i)
                if (bmp.Palette.Entries[i].A != 0 && bmp.Palette.Entries[i].A != 255)
                    return true;

            return false;
        }

        /// <summary>
        /// Loads a <see cref="Bitmap"/> from file that doesn't lock the original file.
        /// </summary>
        public static Bitmap LoadNonLockedBitmap(string path)
        {
            return BitmapHelper.LoadBitmap(path);
        }

        /// <summary>
        /// Loads a <see cref="Bitmap"/> from stream, strictly keeping image format.
        /// Suitable for loading indexed PNGs with varied palette size.
        /// <returns></returns>
        public static Bitmap LoadBitmapKeepingFormat(Stream stream)
        {
            return BitmapHelper.LoadBitmap(stream);
        }
    }
}
