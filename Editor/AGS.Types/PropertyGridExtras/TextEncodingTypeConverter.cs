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
            // For the time being, add only one "generic" value for ASCII/ANSI;
            // it is possible to add exact ansi page names later, if necessary
            _values.Add(Encoding.UTF8.WebName, "Unicode (UTF-8)");
            _values.Add("ANSI", "ASCII / ANSI");
        }

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _values;
        }
    }
}
