using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;

namespace AGS.Types
{
    [Serializable]
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("TextColor")]
    public class GUITextBox : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "TextBox";
        public const string SCRIPT_CLASS_TYPE = "TextBox";

        public GUITextBox(int x, int y, int width, int height)
            : base(x, y, width, height)
        {
            ShowBorder = true; // border is visible by default
            _font = 0;
            _textColor = 0;
            PaddingX = 1;
            PaddingY = 1;
            _textAlign = FrameAlignment.TopLeft;
        }

        public GUITextBox(XmlNode node) : base(node)
        {
        }

        public GUITextBox() { }

        private string _text = string.Empty;
        private int _font;
        private int _textColor;
        private FrameAlignment _textAlign = FrameAlignment.TopLeft;
        private string _onActivate = string.Empty;

        [Description("Script function to run when return is pressed in the text box")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty(), AGSDefaultEventProperty()]
        [ScriptFunction("OnActivate", "GUIControl *control")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnActivate
        {
            get { return _onActivate; }
            set { _onActivate = value; }
        }

        [Description("Colour of the text")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        [SerializeAsHex]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
        }

        [Description("Font to use for the text")]
        [Category("Appearance")]
        [TypeConverter(typeof(FontTypeConverter))]
        public int Font
        {
            get { return _font; }
            set { _font = value; }
        }

        [Description("Position on the control where the text is displayed")]
        [Category("Appearance")]
        public FrameAlignment TextAlignment
        {
            get { return _textAlign; }
            set { _textAlign = value; }
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

        [Browsable(false)]
        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

    }
}
