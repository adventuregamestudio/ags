using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;

namespace UpdateCPPVersion
{
	/// <summary>
	/// The MSI doesn't bother to install the DLL files if there is already
	/// one there with the same version number, therefore we have to force
	/// an update of the version of all DLL's -- this hack is to update the C++ one.
	/// </summary>
	class Program
	{
		private const string CPP_RESOURCE_FILE = @"..\..\..\AGS.Native\NativeDLL.rc";

		static void Main(string[] args)
		{
            string executingDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string rcFileName = Path.Combine(executingDir, CPP_RESOURCE_FILE);
            string fileName = Path.Combine(executingDir, @"..\..\..\AGS.Types\bin\Release\AGS.Types.dll");
			//string fileName = args[0];
			Assembly assembly = Assembly.LoadFile(fileName);
			Version typesVersion = assembly.GetName().Version;

			StreamReader sr = new StreamReader(rcFileName);
			string fileContents = sr.ReadToEnd();
			sr.Close();

			if (!fileContents.Contains(typesVersion.ToString()))
			{
				fileContents = Regex.Replace(fileContents, @"\s[0-9]+,[0-9]+,[0-9]+,[0-9]+", " " + typesVersion.Major + "," + typesVersion.Minor + "," + typesVersion.Build + "," + typesVersion.Revision);
				fileContents = Regex.Replace(fileContents, "\\\"" + @"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+", "\"" + typesVersion.ToString());

				StreamWriter sw = new StreamWriter(rcFileName);
				sw.Write(fileContents);
				sw.Close();

				Console.WriteLine("Version updated to " + typesVersion.ToString());
			}
			else
			{
				Console.WriteLine("Version already " + typesVersion.ToString() + "; not updating");
			}
		}
	}
}
