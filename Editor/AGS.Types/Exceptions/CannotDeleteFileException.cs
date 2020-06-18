using System;

namespace AGS.Types
{
    public class CannotDeleteFileException : AGSEditorException
    {
        public CannotDeleteFileException(string message)
            : base(message)
        {
        }

        public CannotDeleteFileException(string message, Exception innerException)
            : base(message, innerException)
        {
        }
    }
}
