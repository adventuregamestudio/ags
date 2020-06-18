using System;

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
