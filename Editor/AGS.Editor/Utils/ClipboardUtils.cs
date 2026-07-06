using System;
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
    }
}
