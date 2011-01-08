using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    public class GUITextWindowEdge : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "TextWindowEdge";
        public const string SCRIPT_CLASS_TYPE = "Button";

        public GUITextWindowEdge(int x, int y, int id)
            : base(x, y, 10, 10)
        {
            _image = 1;
            _id = id;
            _zorder = id;
            _name = string.Empty;
        }

        public GUITextWindowEdge(XmlNode node)
            : base(node)
        {
        }

        public GUITextWindowEdge() { }

        private int _image;

        [Description("Image to be used on this edge/corner of the text window")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = value; }
        }

        public override string ControlType
        {
            get
            {
                return CONTROL_DISPLAY_NAME;
            }
        }

        public override string ScriptClassType
        {
            get
            {
                return SCRIPT_CLASS_TYPE;
            }
        }

		protected override void GetSpritesForControl(List<int> list)
		{
			if (_image > 0) list.Add(_image);
		}

		public override void UpdateSpritesWithMapping(Dictionary<int, int> spriteMapping)
		{
			if (_image > 0) _image = spriteMapping[_image];
		}
	}
}