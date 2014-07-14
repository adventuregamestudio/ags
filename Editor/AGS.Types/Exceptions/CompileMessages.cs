using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class CompileMessages : List<CompileMessage>
    {
        public CompileMessages()
        {
        }

        public bool HasErrors
        {
            get
            {
                foreach (CompileMessage message in this)
                {
                    if (message is CompileError)
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        public List<CompileError> Errors
        {
            get
            {
                List<CompileError> errors = new List<CompileError>();
                foreach (CompileMessage message in this)
                {
                    if (message is CompileError)
                    {
                        errors.Add((CompileError)message);
                    }
                }
                return errors;
            }
        }
    }
}
