using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class PreSaveGameEventArgs
    {
        private bool _saveSucceeded = true;

        public PreSaveGameEventArgs()
        {
        }

        public bool SaveSucceeded
        {
            get { return _saveSucceeded; }
            set { _saveSucceeded = value; }
        }
    }
}
