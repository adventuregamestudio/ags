using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

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

        private const int BITMAPCOREHEADER_SIZE = 12;
        private const int BITMAPINFOHEADER_SIZE = 40;
        private const int BITMAPV2INFOHEADER_SIZE = 52;
        private const int BITMAPV3INFOHEADER_SIZE = 56;
        private const int BITMAPV4INFOHEADER_SIZE = 108;
        private const int COMPRESSION_NONE = 0;
        private const int COMPRESSION_BITFIELDS = 3;

        /// <summary>
        /// Saves image as DIB format of type BITFIELDS.
        /// 
        /// According to the information from https://stackoverflow.com/a/46424800 ,
        /// the 32-bit RGB format (without alpha) is supported for pasting from clipboard
        /// by a wide range of graphic software. Unfortunately, ARGB format is less
        /// supported, so not recommended to be used for sharing with external software.
        /// For this reason we save the DIB with BITMAPCOREHEADER, and not with the more
        /// appropriate BITMAPV3INFOHEADER_SIZE which supports alpha channel.
        /// </summary>
        public static byte[] SaveDIB(Bitmap image, bool withAlpha = false)
        {
            int width = image.Width;
            int height = image.Height;
            int colorDepth = image.GetColorDepth();
            Bitmap useImage = image;
            // Convert 16-bit bitmaps to 24-bit, because these seem to have higher compatibility
            if (colorDepth > 8 && colorDepth < 24)
            {
                Bitmap newBitmap = new Bitmap(image.Width, image.Height, PixelFormat.Format24bppRgb);
                using (Graphics g = Graphics.FromImage(newBitmap))
                    g.DrawImage(image, new Rectangle(0, 0, width, height));
                useImage = newBitmap;
            }

            int compression = colorDepth == 8 ? COMPRESSION_NONE : COMPRESSION_BITFIELDS;
            withAlpha &= (image.PixelFormat == PixelFormat.Format32bppArgb);
            bool withPalette = (colorDepth == 8) && (image.Palette != null);
            int colorsCount = withPalette ? image.Palette.Entries.Length : 0;

            Rectangle sourceRect = new Rectangle(0, 0, useImage.Width, useImage.Height);
            BitmapData sourceData = useImage.LockBits(sourceRect, ImageLockMode.ReadOnly, useImage.PixelFormat);
            IntPtr sourcePos = sourceData.Scan0;

            // BITMAPINFOHEADER struct for DIB.
            // NOTE: we are technically using BITMAPV2INFOHEADER to store BITFIELDS data,
            // but tag it as BITMAPINFOHEADER instead, because not every software understands
            // BITMAPV2INFOHEADER. BITFIELDS compression type actually instructs to use
            // same header extension as BITMAPV2INFOHEADER, so their sizes match.
            int realhdrSize = withAlpha ? BITMAPV3INFOHEADER_SIZE : BITMAPV2INFOHEADER_SIZE;
            int reportHdrSize = withAlpha ? BITMAPV3INFOHEADER_SIZE : BITMAPINFOHEADER_SIZE;
            // Classic stride: fit within blocks of 4 bytes.
            int bitCount = Image.GetPixelFormatSize(sourceData.PixelFormat);
            int destStride = (((((bitCount * width) + 7) / 8) + 3) / 4) * 4;
            int widthBytes = ((bitCount * width) + 7) / 8;
            int dibDataSize = destStride * sourceData.Height;
            byte[] pixelBuffer = new byte[dibDataSize];
            // Copy line by line in reverse order, because DIB has image inverted vertically
            for (int y = 0, pxIndex = destStride * height - destStride; y < height; ++y, pxIndex -= destStride)
            {
                Marshal.Copy(sourcePos, pixelBuffer, pxIndex, widthBytes);
                sourcePos = new IntPtr(sourcePos.ToInt64() + sourceData.Stride);
            }

            useImage.UnlockBits(sourceData);
            if (useImage != image)
                useImage.Dispose();

            byte[] dibBuffer = new byte[realhdrSize + dibDataSize];
            using (MemoryStream ms = new MemoryStream(dibBuffer))
            using (BinaryWriter writer = new BinaryWriter(ms))
            {
                // BITMAPINFOHEADER
                writer.Write(reportHdrSize);//Int32 biSize;
                writer.Write(width);        //Int32 biWidth;
                writer.Write(height);       //Int32 biHeight;
                writer.Write((short)1);     //Int16 biPlanes;
                writer.Write((short)colorDepth); //Int16 biBitCount;
                writer.Write(compression);  //BITMAPCOMPRESSION biCompression
                writer.Write(dibDataSize);  //Int32 biSizeImage;
                writer.Write(0);            //Int32 biXPelsPerMeter
                writer.Write(0);            //Int32 biYPelsPerMeter
                writer.Write(colorsCount);  //Int32 biClrUsed
                writer.Write(colorsCount);  //Int32 biClrImportant
                if (compression == COMPRESSION_BITFIELDS)
                {
                    // BITMAPV2INFOHEADER
                    writer.Write(0x00FF0000);   //BITFIELDS: Red
                    writer.Write(0x0000FF00);   //BITFIELDS: Green
                    writer.Write(0x000000FF);   //BITFIELDS: Blue
                }
                if ((compression == COMPRESSION_BITFIELDS) && withAlpha)
                {
                    // BITMAPV3INFOHEADER
                    writer.Write(0xFF000000);   //BITFIELDS: Alpha
                }
                // Palette follows header (if present)
                if (withPalette)
                {
                    foreach (var entry in image.Palette.Entries)
                    {
                        // CHECKME: is this a correct order of rgb here?
                        writer.Write((byte)entry.B);
                        writer.Write((byte)entry.G);
                        writer.Write((byte)entry.R);
                        writer.Write((byte)entry.A);
                    }
                }
                // Pixel data
                writer.Write(pixelBuffer);
            }
            return dibBuffer;
        }

        private struct BITMAPHEADER
        {
            public int HdrSize;
            public int Width;
            public int Height;
            public int ColorDepth;
            public int Compression;
            public int DibSize;
            public int ColorsCount;
            public int RedBits;
            public int GreenBits;
            public int BlueBits;
            public int AlphaBits;
        }

        public static Bitmap LoadDIB(MemoryStream dibStream)
        {
            BITMAPHEADER header = new BITMAPHEADER();
            Color[] palette = null;
            byte[] pixelBuffer = null;

            // Read the DIB data into header, palette and pixelBuffer
            using (BinaryReader reader = new BinaryReader(dibStream))
            {
                // BITMAPINFOHEADER
                header.HdrSize = reader.ReadInt32();        //Int32 biSize;
                header.Width = reader.ReadInt32();          //Int32 biWidth;
                header.Height = reader.ReadInt32();         //Int32 biHeight;
                reader.ReadInt16();                         //Int16 biPlanes;
                header.ColorDepth = reader.ReadInt16();     //Int16 biBitCount;
                header.Compression = reader.ReadInt32();    //BITMAPCOMPRESSION biCompression;
                header.DibSize = reader.ReadInt32();        //Int32 biSizeImage;
                reader.ReadInt32();                         //Int32 biXPelsPerMeter
                reader.ReadInt32();                         //Int32 biYPelsPerMeter
                header.ColorsCount = reader.ReadInt32();    //Int32 biClrUsed
                reader.ReadInt32();                         //Int32 biClrImportant
                if ((header.HdrSize >= BITMAPV2INFOHEADER_SIZE) ||
                    (header.Compression == COMPRESSION_BITFIELDS))
                {
                    // BITMAPV2INFOHEADER
                    header.RedBits = reader.ReadInt32();    //BITFIELDS: Red
                    header.GreenBits = reader.ReadInt32();  //BITFIELDS: Green
                    header.BlueBits = reader.ReadInt32();   //BITFIELDS: Blue
                }
                if (header.HdrSize >= BITMAPV3INFOHEADER_SIZE)
                {
                    // BITMAPV3INFOHEADER
                    header.AlphaBits = reader.ReadInt32();  //BITFIELDS: Alpha
                }
                // Skip any extended header format
                if (header.HdrSize > BITMAPV3INFOHEADER_SIZE)
                {
                    reader.BaseStream.Seek(header.HdrSize - BITMAPV3INFOHEADER_SIZE, SeekOrigin.Current);
                }
                // Palette goes after header (if present)
                if (header.ColorsCount > 0)
                {
                    palette = new Color[header.ColorsCount];
                    for (int i = 0; i < header.ColorsCount; ++i)
                    {
                        // CHECKME: is this a correct order of rgb here?
                        byte g = reader.ReadByte();
                        byte b = reader.ReadByte();
                        byte r = reader.ReadByte();
                        byte a = reader.ReadByte();
                        palette[i] = Color.FromArgb(a, r, g, b);
                    }
                }
                pixelBuffer = new byte[header.DibSize];
                reader.Read(pixelBuffer, 0, header.DibSize);
            }

            // Construct a new Bitmap out of the read data
            PixelFormat pxFormat = PixelFormat.Undefined;
            switch (header.ColorDepth)
            {
                case 8: pxFormat = PixelFormat.Format8bppIndexed; break;
                case 24: pxFormat = PixelFormat.Format24bppRgb; break;
                case 32: pxFormat = PixelFormat.Format32bppArgb; break;
                default: break;
            }
            if (pxFormat == PixelFormat.Undefined)
                return null; // not supported

            Bitmap image = new Bitmap(header.Width, header.Height, pxFormat);
            Rectangle destRect = new Rectangle(0, 0, header.Width, header.Height);
            BitmapData destData = image.LockBits(destRect, ImageLockMode.ReadOnly, image.PixelFormat);
            IntPtr destPos = destData.Scan0;
            // Classic stride: fit within blocks of 4 bytes.
            int srcStride = (((((header.ColorDepth * header.Width) + 7) / 8) + 3) / 4) * 4;
            int widthBytes = ((header.ColorDepth * header.Width) + 7) / 8;

            // Copy line by line in reverse order, because DIB has image inverted vertically
            for (int y = 0, pxIndex = srcStride * header.Height - srcStride; y < header.Height; ++y, pxIndex -= srcStride)
            {
                Marshal.Copy(pixelBuffer, pxIndex, destPos, widthBytes);
                destPos = new IntPtr(destPos.ToInt64() + destData.Stride);
            }
            image.UnlockBits(destData);

            // Apply palette if necessary
            if (palette != null)
            {
                for (int i = 0; i < palette.Length && i < image.Palette.Entries.Length; ++i)
                    image.Palette.Entries[i] = palette[i];
            }

            return image;
        }

        public static Bitmap LoadDIB(byte[] dibBuffer)
        {
            return LoadDIB(new MemoryStream(dibBuffer));
        }
    }

    /// <summary>
    /// BitmapExtensions is a collection of helpers for operations with bitmaps.
    /// </summary>
    public static class BitmapExtensions
    {
        /// <summary>
        /// Gets color depth of the image, in bits per pixel.
        /// </summary>
        /// <param name="bmp">The image to get the color depth from.</param>
        /// <returns>Color depth, in bits per pixel.</returns>
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
