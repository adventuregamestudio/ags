using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// WARNING: this interface is considered deprecated in practice
    /// and has been cut out of the AGS Editor. Do restore the interface
    /// methods if it's found that any plugins are requiring them,
    /// but implement using placeholders
    /// </summary>
    public interface ISourceControlProvider : IDisposable
    {
    }
}
