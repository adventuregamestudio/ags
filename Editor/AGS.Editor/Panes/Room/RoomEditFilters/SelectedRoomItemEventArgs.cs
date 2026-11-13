using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class SelectedRoomItemEventArgs : EventArgs
    {
        public SelectedRoomItemEventArgs(string itemID)
        {
            ItemID = itemID;
        }

        /// <summary>
        /// The ID of the room item in its layer's DesignItems.
        /// </summary>
        public string ItemID { get; private set; }
    }
}
