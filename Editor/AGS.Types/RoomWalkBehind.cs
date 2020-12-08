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
        private int _id;
        private int _baseline;

        public RoomWalkBehind()
        {
        }

        public RoomWalkBehind(XmlNode node) : this()
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        [Description("The ID number of the walk-behind area")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
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
            get { return "Walk-behind area ID " + _id; }
        }

        public void ToXml(XmlTextWriter writer) => SerializeUtils.SerializeToXML(this, writer);
    }
}
