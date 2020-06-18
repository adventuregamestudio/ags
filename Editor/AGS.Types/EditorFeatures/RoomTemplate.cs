using System.Drawing;

namespace AGS.Types
{
	public class RoomTemplate : BaseTemplate
	{
		public RoomTemplate(string fileName, Icon icon)
			: base(fileName, icon)
		{
		}

		public RoomTemplate(string fileName, Icon icon, string friendlyName)
			: base(fileName, icon)
		{
			_friendlyName = friendlyName;
		}
	}
}
