using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Thrown if you attempt to delete a sprite that is still in use.
    /// </summary>
    public class SpriteInUseException : AGSEditorException
    {
        public SpriteInUseException(string message)
            : base(message)
        {
        }
    }
}
