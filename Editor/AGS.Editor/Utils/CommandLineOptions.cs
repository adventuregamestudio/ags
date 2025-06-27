using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// CommandLineOptions parses command-line arguments and gathers a list of options.
    /// It only provides the options, it's up to the application to decide how to interpret them.
    /// </summary>
	public class CommandLineOptions
	{
		private bool _compileAndExit = false;
		private bool _templateSaveAndExit = false;
		private string _projectPath = string.Empty;
        private List<string> _unknownArgs = new List<string>();

		private void Parse(string[] args)
		{
			_compileAndExit = false;
			_templateSaveAndExit = false;

			foreach (string arg in args)
			{
				if (arg.ToLower() == "/compile")
				{
					_compileAndExit = true;
				}
				else if (arg.ToLower() == "/maketemplate")
				{
					_templateSaveAndExit = true;
				}
				else if (arg.StartsWith("/") || arg.StartsWith("-"))
				{
                    _unknownArgs.Add(arg);
				}
				else
				{
                    _projectPath = arg;
                }
			}
		}

        /// <summary>
        /// Constructs a set of options with default values.
        /// </summary>
        public CommandLineOptions()
        {
        }

        /// <summary>
        /// Constructs a set of options assigned from the command line args
        /// </summary>
        public CommandLineOptions(string[] args)
		{
			Parse(args);
		}

		public bool CompileAndExit 
		{
			get { return _compileAndExit; }
		}

		public bool TemplateSaveAndExit
		{
			get { return _templateSaveAndExit;  }
		}

		public string ProjectPath
		{
			get { return _projectPath; }
		}

        /// <summary>
        /// Tells if the application is requested to perform a single operation
        /// in an autonomous mode and exit.
        /// </summary>
        public bool AutoOperationRequested
        {
            get { return CompileAndExit || TemplateSaveAndExit; }
        }

        public List<string> UnknownArgs
        {
            get { return _unknownArgs; }
        }
	}
}
