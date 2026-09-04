﻿using System;
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
            // We do not support anything besides UTF-8 now in AGS 4.x
            _values.Add(Encoding.UTF8.WebName, "Unicode (UTF-8)");
        }

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _values;
        }
    }
}
