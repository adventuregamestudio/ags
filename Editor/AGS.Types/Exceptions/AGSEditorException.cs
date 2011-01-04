using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class AGSEditorException : ApplicationException
    {
        public AGSEditorException(string message) : base(message)
        {
        }

        public AGSEditorException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}
