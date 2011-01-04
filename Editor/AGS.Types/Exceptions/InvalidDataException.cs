using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class InvalidDataException : ApplicationException
    {
        public InvalidDataException(string message)
            : base(message)
        {
        }

        public InvalidDataException(string message, Exception innerException)
            : base(message, innerException)
        {
        }
    }
}
