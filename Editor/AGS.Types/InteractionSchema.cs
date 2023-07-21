using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class InteractionSchema
    {
        private string[] _eventNames;
        private string[] _functionSuffixes;
        private string[] _functionParameterLists;

        public InteractionSchema(string[] eventNames, string[] functionSuffixes, string functionParameterList)
        {
            _eventNames = eventNames;
            _functionSuffixes = functionSuffixes;
            _functionParameterLists = new string[eventNames.Length];
            for (int i = 0; i < eventNames.Length; ++i)
                _functionParameterLists[i] = functionParameterList;
        }

        public InteractionSchema(string[] eventNames, string[] functionSuffixes, string[] functionParameterLists)
        {
            _eventNames = eventNames;
            _functionSuffixes = functionSuffixes;
            _functionParameterLists = functionParameterLists;
        }

        public string[] EventNames
        {
            get { return _eventNames; }
        }

        public string[] FunctionSuffixes
        {
            get { return _functionSuffixes; }
        }

        public string[] FunctionParameterLists
        {
            get { return _functionParameterLists; }
        }
    }
}
