﻿using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("AGSTypes")]
[assembly: AssemblyDescription("AGS Editor Support File")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("Chris Jones et al.")]
[assembly: AssemblyProduct("Adventure Game Studio")]
[assembly: AssemblyCopyright(AGS.Types.Version.AGS_EDITOR_COPYRIGHT)]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]
[assembly: ComVisible(false)]
[assembly: AssemblyVersion(AGS.Types.Version.AGS_EDITOR_VERSION)]
[assembly: AssemblyFileVersion(AGS.Types.Version.AGS_EDITOR_VERSION)]

namespace AGS.Types
{
	public class Version
	{
		public static readonly bool IS_BETA_VERSION = true;
		public const string AGS_EDITOR_DATE = "December 2013";
		public const string AGS_EDITOR_FRIENDLY_VERSION = "3.3.0";
        public const string AGS_EDITOR_VERSION = "3.3.0.1152";
        public const string AGS_EDITOR_COPYRIGHT = "Copyright © 2006-2011 Chris Jones and 2011-2013 others.";
	}
}