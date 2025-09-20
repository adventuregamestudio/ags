using System;

namespace AGS.Types
{
    public class ScriptFunctionAttribute : Attribute
    {
        private string _suffix;
        private string _parameters;

        public ScriptFunctionAttribute(string parameters)
            : base()
        {
            _parameters = parameters;
        }

        public ScriptFunctionAttribute(string suffix, string parameters)
            : base()
        {
            _suffix = suffix;
            _parameters = parameters;
        }

        public string Suffix
        {
            get { return _suffix; }
        }

        public string Parameters
        {
            get { return _parameters; }
        }
    }
}
