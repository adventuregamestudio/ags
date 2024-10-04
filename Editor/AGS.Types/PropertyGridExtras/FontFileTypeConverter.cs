using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class FontFileTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        private static Dictionary<string, string> _possibleValues = new Dictionary<string, string>();
        private static IList<FontFile> _fontFiles = null;

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        public static void SetFontFileList(IList<FontFile> fontFiles)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _fontFiles = fontFiles;
            RefreshFontFileList();
        }

        public static void RefreshFontFileList()
        {
            if (_fontFiles == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            _possibleValues.Add("", "< None >"); // empty font file is a valid value
            foreach (FontFile font in _fontFiles)
            {
                _possibleValues.Add(font.FileName, font.FileName);
            }
        }
    }
}
