using System;

namespace AGS.Types
{
    public class InteractionsAttribute : Attribute
    {
        private string _parameters;

        public InteractionsAttribute(string parameters)
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
