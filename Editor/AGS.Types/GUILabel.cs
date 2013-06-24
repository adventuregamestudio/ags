using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;

namespace AGS.Types
{
    [Serializable]
    public class GUILabel : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "Label";
        public const string SCRIPT_CLASS_TYPE = "Label";
        public const int MAX_TEXT_LENGTH = 2047;

        public GUILabel(int x, int y, int width, int height)
            : base(x, y, width, height)
        {
            _text = "New Label";
            _textAlign = FrameAlignment.TopLeft;
        }

        public GUILabel(XmlNode node) : base(node)
        {
        }

        public GUILabel() { } 

        private string _text;
        private int _font;
        private int _textColor;
        private FrameAlignment _textAlign;

        [Description("Position on the label where the text is displayed")]
        [Category("Appearance")]
        public FrameAlignment TextAlignment
        {
            get { return _textAlign; }
            set { _textAlign = value; }
        }

        [Description("AGS Colour Number of the label text")]
        [Category("Appearance")]
        [DisplayName("TextColourNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
        }

        [Description("Colour of the label text")]
        [Category("Appearance")]
        [DisplayName("TextColor")]
        [RefreshProperties(RefreshProperties.All)]
        [AGSNoSerialize]
        public Color TextColorRGB
        {
            get
            {
                return new AGSColor(_textColor).ToRgb();
            }
            set
            {
                _textColor = new AGSColor(value).ColorNumber;
            }
        }

        [Description("Font to use for the text on this label")]
        [Category("Appearance")]
        [TypeConverter(typeof(FontTypeConverter))]
        public int Font
        {
            get { return _font; }
            set { _font = value; }
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

        [Description("The text displayed on the label")]
        [Category("Appearance")]
        public string Text
        {
            get { return _text; }
            set
            {
                if (value.Length > MAX_TEXT_LENGTH)
                {
                    _text = value.Substring(0, MAX_TEXT_LENGTH);
                }
                else
                {
                    _text = value;
                }
            }
        }

    }
}