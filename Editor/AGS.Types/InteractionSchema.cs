using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class InteractionSchema
    {
        private string _defaultScriptModule = string.Empty;
        private bool _scriptModuleFixed = false;
        private string[] _eventNames;
        private string[] _functionSuffixes;
        private string[] _functionParameterLists;

        public InteractionSchema(string defaultScriptModule, bool scriptModuleFixed,
            string[] eventNames, string[] functionSuffixes, string functionParameterList)
        {
            _eventNames = eventNames;
            _functionSuffixes = functionSuffixes;
            _functionParameterLists = new string[eventNames.Length];
            for (int i = 0; i < eventNames.Length; ++i)
                _functionParameterLists[i] = functionParameterList;
            _defaultScriptModule = defaultScriptModule;
            _scriptModuleFixed = scriptModuleFixed;
        }

        public InteractionSchema(string defaultScriptModule, bool scriptModuleFixed,
            string[] eventNames, string[] functionSuffixes, string[] functionParameterLists)
        {
            _eventNames = eventNames;
            _functionSuffixes = functionSuffixes;
            _functionParameterLists = functionParameterLists;
            _defaultScriptModule = defaultScriptModule;
            _scriptModuleFixed = scriptModuleFixed;
        }

        public string DefaultScriptModule
        {
            get { return _defaultScriptModule; }
        }

        public bool ScriptModuleFixed
        {
            get { return _scriptModuleFixed; }
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
