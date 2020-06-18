using System;

namespace AGS.Types
{
    public class ScriptFunctionParametersAttribute : Attribute
    {
        private string _parameters;

        public ScriptFunctionParametersAttribute(string parameters)
            : base()
        {
            _parameters = parameters;
        }

        public string Parameters
        {
            get { return _parameters; }
        }
    }
}
