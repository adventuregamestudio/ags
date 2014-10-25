using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface ICompiledScript : IDisposable
    {
        /// <summary>
        /// Writes this compiled script's data to the file stream.
        /// NOTE: This function should be considered temporary and will likely be removed
        /// as the compilation process is further refactored.
        /// </summary>
        void Write(System.IO.FileStream ostream, System.String scriptFileName);
    }
}
