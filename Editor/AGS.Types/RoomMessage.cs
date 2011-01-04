using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class RoomMessage
    {
        private int _id;
        private string _text = string.Empty;
        private bool _showAsSpeech = false;
        private int _characterID = 0;
        private bool _displayNextMessageAfter = false;
        private bool _autoRemoveAfterTime = false;

        public RoomMessage(int id)
        {
            _id = id;
        }

        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

        public bool ShowAsSpeech
        {
            get { return _showAsSpeech; }
            set { _showAsSpeech = value; }
        }

        public int CharacterID
        {
            get { return _characterID; }
            set
            {
                if (value < 0) _characterID = 0;
                else _characterID = value;
            }
        }

        public bool DisplayNextMessageAfter
        {
            get { return _displayNextMessageAfter; }
            set { _displayNextMessageAfter = value; }
        }

        public bool AutoRemoveAfterTime
        {
            get { return _autoRemoveAfterTime; }
            set { _autoRemoveAfterTime = value; }
        }
    }
}
