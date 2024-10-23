using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class ScriptCompilerTypeConverter : BaseListSelectTypeConverter<string, string>
    {
        private static Dictionary<string, string> _possibleValues = new Dictionary<string, string>();

        protected override Dictionary<string, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        /// <summary>
        /// Assigns a dictionary of "id / display name".
        /// </summary>
        public static void SetCompilerList(IDictionary<string, string> compilers)
        {
            _possibleValues = new Dictionary<string, string>(compilers);
        }
    }
}
