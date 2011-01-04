using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor.Resources
{
    public class ResourceManager
    {
        private const string DEFAULT_RESOURCE_PATH = "AGS.Editor.Resources.";

        public static Icon GetIcon(string fileName)
        {
            return new Icon(typeof(ResourceManager), fileName);
        }

        public static Bitmap GetBitmap(string fileName)
        {
            return new Bitmap(typeof(ResourceManager), fileName);
        }

        public static Cursor GetCursor(string fileName)
        {
            return new Cursor(typeof(ResourceManager), fileName);
        }

        public static string GetResourceAsString(string fileName)
        {
            Stream input = typeof(ResourceManager).Assembly.GetManifestResourceStream(DEFAULT_RESOURCE_PATH + fileName);
            byte[] rawData = new byte[input.Length];
            input.Read(rawData, 0, rawData.Length);
            input.Close();
            return System.Text.Encoding.ASCII.GetString(rawData);
        }

        public static void CopyFileFromResourcesToDisk(string resourceFileName, string diskFileName)
        {
            Stream input = typeof(ResourceManager).Assembly.GetManifestResourceStream(DEFAULT_RESOURCE_PATH + resourceFileName);
            byte[] rawData = new byte[input.Length];
            input.Read(rawData, 0, rawData.Length);
            input.Close();

            string fileName = Path.Combine(Directory.GetCurrentDirectory(), diskFileName);

            FileStream fs = new FileStream(fileName, FileMode.Create);
            fs.Write(rawData, 0, rawData.Length);
            fs.Close();
        }

        public static void CopyFileFromResourcesToDisk(string resourceFileName)
        {
            CopyFileFromResourcesToDisk(resourceFileName, resourceFileName);
        }
    }
}
