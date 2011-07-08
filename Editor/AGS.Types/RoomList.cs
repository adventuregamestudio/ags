using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class RoomList : List<IRoom>
	{
		public RoomList()
		{
		}

		public void SortByNumber()
		{
			this.Sort();
		}
	}
}
