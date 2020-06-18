using System.Collections.Generic;

namespace AGS.Types
{
    public class GetScriptModuleListEventArgs
    {
        IList<Script> _scripts;

        public GetScriptModuleListEventArgs(IList<Script> scripts)
        {
            _scripts = scripts;
        }

        public IList<Script> Scripts
        {
            get { return _scripts; }
        }
    }
}
