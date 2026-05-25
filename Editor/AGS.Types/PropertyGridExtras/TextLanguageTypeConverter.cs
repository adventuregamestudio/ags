using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;

namespace AGS.Types
{
    public class TextLanguageTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        private static Dictionary<string, string> _possibleValues = new Dictionary<string, string>();

        static TextLanguageTypeConverter()
        {
            _possibleValues.Add("", "(undefined language)");
            var cultures =
                CultureInfo.GetCultures(CultureTypes.SpecificCultures);
            foreach (var ci in cultures)
                _possibleValues.Add(ci.Name, $"{ci.Name} | {{{ci.DisplayName}}}");
        }

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }
    }
}
