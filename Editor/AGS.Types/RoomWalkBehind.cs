using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Baseline")]
    public class RoomWalkBehind : IToXml
    {
        public const string PROPERTY_NAME_SCRIPT_NAME = "ScriptName";

        private int _id;
        private string _scriptName = string.Empty;
        private int _baseline;
        private Room _room;

        public RoomWalkBehind(Room room)
        {
            _room = room;
        }

        public RoomWalkBehind(Room room, XmlNode node) : this(room)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Room Room
        {
            get { return _room; }
        }

        [AGSNoSerialize]
        [Description("The ID number of the walk-behind area")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [DisplayName(PROPERTY_NAME_SCRIPT_NAME)]
        [Description("The script name of the walk-behind")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string ScriptName
        {
            get { return _scriptName; }
            set { _scriptName = Utilities.ValidateScriptName(value); }
        }

        [Description("Characters standing above this baseline will be drawn behind the walk-behind")]
        [Category("Design")]
        public int Baseline
        {
            get { return (_baseline < 0) ? 0 : _baseline; }
            set { _baseline = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Walk-behind area", _id); }
        }
        public void ToXml(XmlTextWriter writer) => SerializeUtils.SerializeToXML(this, writer);
    }
}
