using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IGlobalVariablesController
    {
        /// <summary>
        /// Shows the gloabal variables pane (if it's not shown already),
        /// and selects the global variable. Will show an error if the
        /// variable was not found.
        /// </summary>
        /// <param name="variableName">The name of the variable to show.</param>
        void SelectGlobalVariable(string variableName);
    }
}
