using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public delegate void GetSourceControlFileListHandler(IList<string> fileNames);

    /// <summary>
    /// Operations to handle files under Source Control. If the current game
    /// is not under Source Control, these operations will all work on the
    /// local file system only.
    /// WARNING: this interface is considered deprecated in practice
    /// and has been cut out of the AGS Editor. Do restore the interface
    /// methods if it's found that any plugins are requiring them,
    /// but implement using placeholders
    /// </summary>
    public interface ISourceControlIntegration
    {
    }
}
