using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class UnloadedRoom : IRoom
    {
        private const string ROOM_FILE_NAME_FORMAT = "room{0}.crm";
        private const string ROOM_SCRIPT_FILE_NAME_FORMAT = "room{0}.asc";

		public const int NON_STATE_SAVING_INDEX = 300;
		public const int HIGHEST_ROOM_NUMBER_ALLOWED = 999;
		public const string PROPERTY_NAME_DESCRIPTION = "Description";
		public const string PROPERTY_NAME_NUMBER = "Number";

        protected int _number;
        protected string _description;
        protected Script _script = null;

		public static bool DoRoomFilesExist(int roomNumber)
		{
			if (File.Exists(string.Format(ROOM_FILE_NAME_FORMAT, roomNumber)) ||
				File.Exists(string.Format(ROOM_SCRIPT_FILE_NAME_FORMAT, roomNumber)))
			{
				return true;
			}
			return false;
		}

        public UnloadedRoom(int roomNumber)
        {
            _number = roomNumber;
        }

		[DisplayName(PROPERTY_NAME_NUMBER)]
        [Description("The ID number of this room")]
        [Category("Design")]
        public int Number
        {
            get { return _number; }
            set 
			{ 
				_number = value;
				if (_script != null)
				{
					_script.FileName = this.ScriptFileName;
				}
			}
        }

        [DisplayName(PROPERTY_NAME_DESCRIPTION)]
        [Description("The room's description")]
        [Category("Design")]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

        [Description("The filename containing this room")]
        [Category("Design")]
        public string FileName
        {
            get { return string.Format(ROOM_FILE_NAME_FORMAT, _number); }
        }

		[Description("Whether the state of the room is saved when the player leaves the room and comes back")]
		[Category("Design")]
		[ReadOnly(true)]
		public bool StateSaving
		{
			get { return (_number < NON_STATE_SAVING_INDEX); }
		}

		[Browsable(false)]
        public string ScriptFileName
        {
            get { return string.Format(ROOM_SCRIPT_FILE_NAME_FORMAT, _number); }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public Script Script
        {
            get { return _script; }
            set { _script = value; }
        }

        public void LoadScript()
        {
			if (_script == null)
			{
				_script = new Script(this.ScriptFileName, string.Empty, false);
			}
            _script.LoadFromDisk();
        }

        public void UnloadScript()
        {
            _script = null;
        }

        public UnloadedRoom(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public int CompareTo(IRoom other)
        {
            return _number.CompareTo(other.Number);
        }
    }
}
