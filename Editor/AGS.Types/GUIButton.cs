using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;

namespace AGS.Types
{    
    [Serializable]
    [DefaultProperty("Image")]
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]    
    public class GUIButton : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "Button";
        public const string SCRIPT_CLASS_TYPE = "Button";
        public const int MAX_TEXT_LENGTH = 49;

        public GUIButton(int x, int y, int width, int height) : base(x, y, width, height)
        {
            _text = "New Button";
            _clickAction = GUIClickAction.RunScript;
            _textAlign = TextAlignment.TopMiddle;
        }

        public GUIButton(XmlNode node) : base(node)
        {
        }

        public GUIButton() : base()
        {
        }

        private string _text;
        private int _image;
        private int _mouseoverImage;
        private int _pushedImage;
        private int _font;
        private int _textColor;
        private TextAlignment _textAlign;
        private bool _clipImage;
        private GUIClickAction _clickAction;
        private int _newModeNumber;
        private string _clickEventHandler = string.Empty;

        [Description("Script function to run when the button is clicked")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventProperty()]
        [ScriptFunctionParameters("GUIControl *control, MouseButton button")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnClick
        {
            get { return _clickEventHandler; }
            set { _clickEventHandler = value; }
        }

        [Description("What action to perform when the button is clicked")]
        [Category("Design")]
        public GUIClickAction ClickAction
        {
            get { return _clickAction; }
            set { _clickAction = value; }
        }

        [Description("The cursor mode to change to (only valid if ClickAction is SetCursorMode)")]
        [Category("Design")]
        public int NewModeNumber
        {
            get { return _newModeNumber; }
            set { _newModeNumber = value; }
        }

        [Description("If true, the button image will not be allowed to overflow outside the size of the button")]
        [Category("Appearance")]
        public bool ClipImage
        {
            get { return _clipImage; }
            set { _clipImage = value; }
        }

        [Description("Position on the button where the text is displayed")]
        [Category("Appearance")]
        public TextAlignment TextAlignment
        {
            get { return _textAlign; }
            set { _textAlign = value; }
        }

        [Description("AGS Colour Number of the button text")]
        [Category("Appearance")]
        [DisplayName("TextColourNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
        }

        [Description("Colour of the button text")]
        [Category("Appearance")]
        [DisplayName("TextColour")]
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

        [Description("Font to use for the text on this button")]
        [Category("Appearance")]
        [TypeConverter(typeof(FontTypeConverter))]
        public int Font
        {
            get { return _font; }
            set { _font = value; }
        }

        [Description("Image to display when the player clicks the button")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int PushedImage
        {
            get { return _pushedImage; }
            set 
            { 
                _pushedImage = value;
                if (_pushedImage < 0) _pushedImage = 0;
            }
        }

        [Description("Image to display when the player moves their mouse over the button")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int MouseoverImage
        {
            get { return _mouseoverImage; }
            set 
            { 
                _mouseoverImage = value;
                if (_mouseoverImage < 0) _mouseoverImage = 0;
            }
        }

        [Description("Optional image to be displayed on the button")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set 
            { 
                _image = value;
                if (_image < 0) _image = 0;
            }
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

        [Description("The text displayed on the button")]
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

        protected override void GetSpritesForControl(List<int> list)
        {
            if (_image > 0) list.Add(_image);
            if (_mouseoverImage > 0) list.Add(_mouseoverImage);
            if (_pushedImage > 0) list.Add(_pushedImage);
        }

        public override void UpdateSpritesWithMapping(Dictionary<int, int> spriteMapping)
        {
            if (_image > 0) _image = spriteMapping[_image];
            if (_mouseoverImage > 0) _mouseoverImage = spriteMapping[_mouseoverImage];
            if (_pushedImage > 0) _pushedImage = spriteMapping[_pushedImage];
        }
    }
}