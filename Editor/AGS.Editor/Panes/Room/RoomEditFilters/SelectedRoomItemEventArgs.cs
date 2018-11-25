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

        public string Item { get; private set; }
    }
}
