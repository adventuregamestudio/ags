using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	/// <summary>
	/// The minimum version of AGS that this plugin will work with.
	/// </summary>
	[AttributeUsage(AttributeTargets.Class)]
	public class RequiredAGSVersionAttribute : Attribute
	{
		private string _requiredAGSVersion;

		/// <summary>
		/// Specifies the minimum AGS version required by this plugin
		/// </summary>
		/// <param name="requiredVersion">Minimum version number, in format a.b.c.d (eg. 3.0.0.0)</param>
		public RequiredAGSVersionAttribute(string requiredVersion)
		{
			_requiredAGSVersion = requiredVersion;
		}

		public string RequiredVersion
		{
			get { return _requiredAGSVersion; }
		}
	}
}
