using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class ScriptListTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        private static Dictionary<string, string> _possibleValues = new Dictionary<string, string>();
        private static ScriptsAndHeaders _scripts = null;

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        public static void SetScriptList(ScriptsAndHeaders scripts)
        {
            // Keep a reference to the list so it can be updated whenever we need to
            _scripts = scripts;
            RefreshScriptList();
        }

        public static void RefreshScriptList()
        {
            if (_scripts == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            foreach (ScriptAndHeader script in _scripts.OrderBy(s => s.Name))
            {
                _possibleValues.Add(script.Script.FileName, script.Script.FileName);
            }
        }
    }
}
