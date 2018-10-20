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
            _text = string.Empty;
            _showBorder = true;
            _font = 0;
        }

        public GUITextBox(XmlNode node) : base(node)
        {
        }

        public GUITextBox() { }

        private string _text;
        private int _font;
        private int _textColor;
        private bool _showBorder;
        private string _activateEventHandler = string.Empty;

        [Description("Script function to run when return is pressed in the text box")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventProperty()]
        [ScriptFunctionParameters("GUIControl *control")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnActivate
        {
            get { return _activateEventHandler; }
            set { _activateEventHandler = value; }
        }

        [Description("AGS Colour Number of the text")]
        [Category("Appearance")]
        [DisplayName("TextColourNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
        }

        [Description("Colour of the text")]
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

        [Description("Font to use for the text")]
        [Category("Appearance")]
        [TypeConverter(typeof(FontTypeConverter))]
        public int Font
        {
            get { return _font; }
            set { _font = value; }
        }

        [Description("Determines whether the border of the text box is drawn")]
        [Category("Appearance")]
        public bool ShowBorder
        {
            get { return _showBorder; }
            set { _showBorder = value; }
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