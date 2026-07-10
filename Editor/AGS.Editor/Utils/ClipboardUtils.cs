using AGS.Editor.Utils;
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;

namespace AGS.Editor
{
    public static class ClipboardUtils
    {
        public static string GetFormatName(Type type)
        {
            return type.FullName;
        }

        public static string GetFormatName(object thing)
        {
            return thing.GetType().FullName;
        }

        public static void CopyToClipboard(ICloneable thing)
        {
            object thingCopy = thing.Clone();
            MoveToClipboard(thingCopy, GetFormatName(thing));
        }

        public static void CopyToClipboard(ICloneable thing, string formatName)
        {
            object thingCopy = thing.Clone();
            MoveToClipboard(thingCopy, formatName);
        }

        public static void MoveToClipboard(object thing)
        {
            MoveToClipboard(thing, GetFormatName(thing));
        }

        public static void MoveToClipboard(object thing, string formatName)
        {
            DataFormats.Format format = DataFormats.GetFormat(formatName);
            IDataObject dataObj = new DataObject();
            dataObj.SetData(format.Name, false, thing);
            Clipboard.SetDataObject(dataObj, true);
        }

        public static object PasteFromClipboard(Type expectType)
        {
            return PasteFromClipboard(expectType, GetFormatName(expectType));
        }

        public static object PasteFromClipboard(Type expectType, string formatName)
        {
            IDataObject dataObj = Clipboard.GetDataObject();
            if (dataObj != null && dataObj.GetDataPresent(formatName))
            {
                object thing = dataObj.GetData(formatName);
                if (thing != null && thing.GetType() == expectType)
                    return thing;
            }
            return null;
        }

        public static bool IsAvailableOnClipboard(Type expectType)
        {
            return IsAvailableOnClipboard(GetFormatName(expectType));
        }

        public static bool IsAvailableOnClipboard(string formatName)
        {
            IDataObject dataObj = Clipboard.GetDataObject();
            if (dataObj != null)
                return dataObj.GetDataPresent(formatName);
            return false;
        }

        /// <summary>
        /// Copies the given image to the clipboard, as few separate formats.
        /// 
        /// For some reason, sending Bitmap object to clipboard will cause image to loose alpha channel;
        /// in fact, it replaces alpha transparency with hues of gray. PNG stream does support the alpha
        /// channel, but may not be accepted by some of the graphic editing software out there.
        /// DIB stream is peculiar: true 32-bit ARGB DIB is apparently not recognized by most image editors
        /// (at least not when coming from a clipboard), so it's safer to use 32-bit RGB instead. That
        /// is not reliable in terms of alpha channel (it's kept in data, even though the format is not
        /// specified as having one), and alpha transparency may be interpreted into "magic pink" color
        /// when pasting from clipboard (depends on software), but is still better than Bitmap.
        /// 
        /// If desired, then more formats could be added here.
        /// 
        /// Explanation of a problem:
        /// https://stackoverflow.com/questions/44177115/copying-from-and-to-clipboard-loses-image-transparency
        /// Idea of a solution, and small portion of the code was taken from here:
        /// https://stackoverflow.com/a/46424800
        /// </summary>
        public static void SetImage(Bitmap image)
        {
            IDataObject dataObj = new DataObject();
            using (MemoryStream pngMemStream = new MemoryStream())
            using (MemoryStream dibMemStream = new MemoryStream())
            {
                // Copy standard bitmap, without transparency support
                dataObj.SetData(DataFormats.Bitmap, true, image);
                // As PNG. This is ideal format, but not every application recognizes it.
                image.Save(pngMemStream, ImageFormat.Png);
                dataObj.SetData("PNG", false, pngMemStream);
                // As DIB. Must write without alpha bit mask, for maximal compatibility
                Byte[] dibData = BitmapHelper.SaveDIB(image, false);
                dibMemStream.Write(dibData, 0, dibData.Length);
                dataObj.SetData(DataFormats.Dib, false, dibMemStream);
                Clipboard.SetDataObject(dataObj, true);
            }
        }


        /// <summary>
        /// Retrieves a Bitmap image from clipboard, trying few supported formats.
        /// 
        /// Explanation of a problem:
        /// https://stackoverflow.com/questions/44177115/copying-from-and-to-clipboard-loses-image-transparency
        /// Idea and portions of code are taken from:
        /// https://stackoverflow.com/a/46424800
        /// </summary>
        public static Bitmap GetBitmap()
        {
            DataObject dataObj = Clipboard.GetDataObject() as DataObject;
            if (dataObj == null)
                return null;

            // Order: try PNG, move on to try DIB, then try the normal Bitmap and Image types.
            if (dataObj.GetDataPresent("PNG", false))
            {
                using (MemoryStream pngStream = dataObj.GetData("PNG", false) as MemoryStream)
                {
                    if (pngStream != null)
                    {
                        try
                        {
                            Bitmap image = BitmapHelper.LoadBitmap(pngStream);
                            if (image != null)
                                return image;
                        }
                        catch
                        {
                            // continue to the next attempt
                        }
                    }
                }
            }
            if (dataObj.GetDataPresent(DataFormats.Dib, false))
            {
                using (MemoryStream dibStream = dataObj.GetData(DataFormats.Dib, false) as MemoryStream)
                {
                    if (dibStream != null)
                    {
                        try
                        {
                            Bitmap image = BitmapHelper.LoadDIB(dibStream);
                            if (image != null)
                                return image;
                        }
                        catch
                        {
                            // continue to the next attempt
                        }
                    }
                }
            }
            if (dataObj.GetDataPresent(DataFormats.Bitmap))
                return new Bitmap(dataObj.GetData(DataFormats.Bitmap) as Image);
            if (dataObj.GetDataPresent(typeof(Image)))
                return new Bitmap(dataObj.GetData(typeof(Image)) as Image);
            return null;
        }
    }
}
