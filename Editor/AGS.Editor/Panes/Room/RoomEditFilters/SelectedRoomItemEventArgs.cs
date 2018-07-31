using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class SelectedRoomItemEventArgs : EventArgs
    {
        public SelectedRoomItemEventArgs(string item)
        {
            Item = item;
        }

        /// <summary>
        /// Room item's human-readable name.
        /// </summary>
        public string Item { get; private set; }
    }
}
