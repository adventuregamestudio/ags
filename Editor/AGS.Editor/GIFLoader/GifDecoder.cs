using ImageMagick;
using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace AGS.Editor
{
    public class GifDecoder : IDisposable
    {
        private MagickImageCollection collection;

        public GifDecoder(string fileName, bool coalesce = true)
        {
            switch (new MagickImageInfo(fileName).Format)
            {
                case MagickFormat.Gif:
                case MagickFormat.Gif87:
                    break;
                default:
                    throw new Types.InvalidDataException("Unable to load GIF");
            }

            try
            {
                collection = new MagickImageCollection(fileName);
            }
            catch (MagickException ex)
            {
                throw new Types.InvalidDataException(ex.Message);
            }

            if (coalesce)
            {
                collection.Coalesce();
            }
        }

        public int GetFrameCount()
        {
            return collection.Count;
        }

        public Bitmap GetFrame(int frameNumber)
        {
            return collection[frameNumber].ToBitmap(ImageFormat.Gif);
        }

        public void Dispose()
        {
            collection.Dispose();
        }
    }
}
