using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class MouseCursor
    {
        private string _name;
        private bool _standardMode;
        private int _id;
        private int _image;
        private int _hotspotX, _hotspotY;
        private int _view;
        private bool _animate;
        private bool _animateOnlyOnHotspot;
        private bool _animateOnlyWhenMoving;

        public MouseCursor()
        {
            _standardMode = false;
            _name = string.Empty;
            _image = 0;
            _hotspotX = 0;
            _hotspotY = 0;
            _view = 0;
            _animate = false;
            _animateOnlyOnHotspot = false;
            _animateOnlyWhenMoving = false;
        }

        [Description("The ID number of the cursor")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The X location of the cursor hotspot")]
        [Category("Design")]
        public int HotspotX
        {
            get { return _hotspotX; }
            set { _hotspotX = value; }
        }

        [Description("The Y location of the cursor hotspot")]
        [Category("Design")]
        public int HotspotY
        {
            get { return _hotspotY; }
            set { _hotspotY = value; }
        }

        [Description("This cursor mode should fire interactions via ProcessClick")]
        [Category("Design")]
        public bool StandardMode
        {
            get { return _standardMode; }
            set { _standardMode = value; }
        }

        [Description("Sprite used to display the cursor")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set
            {
                if (value != _image)
                {
                    if (_image != 0)
                    {
                        _hotspotX = 0;
                        _hotspotY = 0;
                    }
                    _image = value;
                }
            }
        }

        [Description("The view used to animate the cursor")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int View
        {
            get { return _view; }
            set { _view = value; }
        }

        [Description("Whether the cursor will animate using the specified view")]
        [Category("Appearance")]
        public bool Animate
        {
            get { return _animate; }
            set { _animate = value; }
        }

        [Description("The cursor will only animate when over a hotspot or object (Animate must be set to true)")]
        [Category("Appearance")]
        public bool AnimateOnlyOnHotspots
        {
            get { return _animateOnlyOnHotspot; }
            set { _animateOnlyOnHotspot = value; }
        }

        [Description("The cursor will only animate when it is moving")]
        [Category("Appearance")]
        public bool AnimateOnlyWhenMoving
        {
            get { return _animateOnlyWhenMoving; }
            set { _animateOnlyWhenMoving = value; }
        }

        [Description("The name of the cursor")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set
            {
                _name = value;
                if (_name.Length > 9) _name = _name.Substring(0, 9);
            }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "Cursor: " + this.Name; }
        }

        [Description("The script ID of the cursor")]
        [Category("Design")]
        public string ScriptID
        {
			get
			{
				string cursorName = string.Empty;
				for (int i = 0; i < _name.Length; i++)
				{
					if (Char.IsLetterOrDigit(_name[i]))
					{
						cursorName += _name[i];
					}
				}
				if (cursorName.Length > 0)
				{
					cursorName = "eMode" + cursorName;
				}
				return cursorName;
			}
        }

        public MouseCursor(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

    }
}
