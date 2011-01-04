using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class InteractionSchema
    {
        private string[] _eventNames;
        private string[] _functionSuffixes;

        public InteractionSchema(string[] eventNames, string[] functionSuffixes)
        {
            _eventNames = eventNames;
            _functionSuffixes = functionSuffixes;
        }

        public string[] EventNames
        {
            get { return _eventNames; }
        }

        public string[] FunctionSuffixes
        {
            get { return _functionSuffixes; }
        }
    }
}
