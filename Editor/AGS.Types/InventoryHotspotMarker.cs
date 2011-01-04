using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class InventoryHotspotMarker
    {
        private InventoryHotspotMarkerStyle _style;
        private int _dotColor;
        private int _crosshairColor;
        private int _spriteSlot;

        public InventoryHotspotMarkerStyle Style
        {
            get { return _style; }
            set { _style = value; }
        }

        public int DotColor
        {
            get { return _dotColor; }
            set { _dotColor = value; }
        }

        public int CrosshairColor
        {
            get { return _crosshairColor; }
            set { _crosshairColor = value; }
        }

        public int Image
        {
            get { return _spriteSlot; }
            set { _spriteSlot = value; }
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void FromXml(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }
    }
}
