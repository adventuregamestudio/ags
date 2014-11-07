using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using AGS.Types;
using System.Xml;

namespace AGS.Types
{
	public class WorkspaceState
	{
		private BuildConfiguration _lastBuildConfiguration = AGS.Types.BuildConfiguration.Unknown;

		public WorkspaceState()
		{
		}

		[Browsable(false)]
		[DefaultValue(BuildConfiguration.Unknown)]
		public BuildConfiguration LastBuildConfiguration
		{
			get { return _lastBuildConfiguration; }
			set { _lastBuildConfiguration = value; }
		}

		public void ToXml(XmlTextWriter writer)
		{
			SerializeUtils.SerializeToXML(this, writer);
		}

		public void FromXml(System.Xml.XmlNode node)
		{
			_lastBuildConfiguration = AGS.Types.BuildConfiguration.Unknown;

			// Allow for earlier versions of the XML
			if (node != null && node.SelectSingleNode("WorkspaceState") != null)
				SerializeUtils.DeserializeFromXML(this, node);
		}
	}
}
