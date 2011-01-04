using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;

namespace AGS.Types
{
    public class FontTypeConverter : BaseListSelectTypeConverter<Font>
    {
        private static Dictionary<int, string> _possibleValues = new Dictionary<int, string>();
        private static IList<Font> _Fonts = null;

        protected override Dictionary<int, string> GetValueList()
        {
            return _possibleValues;
        }

        public static void SetFontList(IList<Font> fonts)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _Fonts = fonts;
            RefreshFontList();
        }

        public static void RefreshFontList()
        {
            if (_Fonts == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            foreach (Font font in _Fonts)
            {
                _possibleValues.Add(font.ID, font.ScriptID);
            }
        }
    }
}