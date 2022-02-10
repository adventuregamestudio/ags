using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public class TextEncodingTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        static Dictionary<string, string> _values;

        static TextEncodingTypeConverter()
        {
            _values = new Dictionary<string, string>();
            foreach (var encInfo in Encoding.GetEncodings())
            {
                if (_values.ContainsKey(encInfo.Name))
                    continue;
                if ((encInfo.CodePage >= 1250 && encInfo.CodePage < 1260))
                {
                    _values.Add(encInfo.Name,
                        string.Format("ANSI {0}: {1}{2}", encInfo.CodePage, encInfo.DisplayName,
                        encInfo.CodePage == Encoding.Default.CodePage ? " [your system's locale]" : ""));
                }
                else if (encInfo.CodePage == 65001) // UTF-8
                {
                    _values.Add(encInfo.Name, "Unicode (UTF-8)");
                }
            }
        }

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _values;
        }
    }
}
