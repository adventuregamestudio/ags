using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace AGS.Types
{
	public class RoomTemplate : BaseTemplate
	{
		public RoomTemplate(string fileName, Icon icon)
			: base(fileName, null, icon)
		{
		}

		public RoomTemplate(string fileName, Icon icon, string friendlyName)
			: base(fileName, null, icon)
		{
			_friendlyName = friendlyName;
		}
	}
}
