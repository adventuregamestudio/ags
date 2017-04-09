using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public class TranslationListTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        private static Dictionary<string, string> _possibleValues = new Dictionary<string, string>();
        private static IList<Translation> _Translations = null;

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        public static void SetTranslationsList(IList<Translation> trans)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _Translations = trans;
            RefreshTranslationsList();
        }

        public static void RefreshTranslationsList()
        {
            if (_Translations == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            _possibleValues.Add("", "Default language");
            foreach (Translation trs in _Translations)
            {
                _possibleValues.Add(trs.Name, trs.Name);
            }
        }
    }
}
