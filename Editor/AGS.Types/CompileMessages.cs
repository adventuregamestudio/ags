using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class CompileMessages : List<ICompileMessage>
	{
		public CompileMessages()
		{
		}

		public bool HasErrors
		{
			get
			{
				foreach (ICompileMessage message in this)
				{
					if (message is CompileError)
					{
						return true;
					}
				}
				return false;
			}
		}

        public bool HasWarnings
        {
            get
            {
                foreach (ICompileMessage message in this)
                {
                    if (message is CompileWarning)
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        public bool HasErrorsOrWarnings
        {
            get
            {
                foreach (ICompileMessage message in this)
                {
                    if ((message is CompileError) || (message is CompileWarning))
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
				foreach (ICompileMessage message in this)
				{
					if (message is CompileError)
					{
						errors.Add((CompileError)message);
					}
				}
				return errors;
			}
		}

        public List<CompileWarning> Warnings
        {
            get
            {
                List<CompileWarning> warnings = new List<CompileWarning>();
                foreach (ICompileMessage message in this)
                {
                    if (message is CompileWarning)
                    {
                        warnings.Add((CompileWarning)message);
                    }
                }
                return warnings;
            }
        }
	}
}
