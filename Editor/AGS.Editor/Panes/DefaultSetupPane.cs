using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    class DefaultRuntimeSetupPane : BigPropertySheet
    {
        public DefaultRuntimeSetupPane()
            : base(Factory.AGSEditor.CurrentGame.DefaultSetup)
        {
        }

        protected override string OnGetHelpKeyword()
        {
            return "Default setup";
        }
    }
}
