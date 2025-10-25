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

        public GUIButton(int x, int y, int width, int height) : base(x, y, width, height)
        {
            _text = "New Button";
            _clickAction = GUIClickAction.RunScript;
            _textAlign = FrameAlignment.TopCenter;
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
        private FrameAlignment _textAlign;
        private bool _wrapText;
        private int _paddingHor = 2;
        private int _paddingVer = 2;
        private bool _clipImage;
        private GUIClickAction _clickAction;
        private int _newModeNumber;
        private string _onClick = string.Empty;

        [Description("Script function to run when the button is clicked")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty(), AGSDefaultEventProperty()]
        [ScriptFunction("GUIControl *control, MouseButton button")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnClick
        {
            get { return _onClick; }
            set { _onClick = value; }
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
        public FrameAlignment TextAlignment
        {
            get { return _textAlign; }
            set { _textAlign = value; }
        }

        [Description("Whether button will wrap text when it exceeds button's width or has new line characters")]
        [Category("Appearance")]
        public bool WrapText
        {
            get { return _wrapText; }
            set { _wrapText = value; }
        }

        [Description("The amount of padding, in pixels, restricting the text's alignment from left and right")]
        [Category("Appearance")]
        [DefaultValue(2)]
        public int TextPaddingHorizontal
        {
            get { return _paddingHor; }
            set { _paddingHor = value; }
        }

        [Description("The amount of padding, in pixels, restricting the text's alignment from top and bottom")]
        [Category("Appearance")]
        [DefaultValue(2)]
        public int TextPaddingVertical
        {
            get { return _paddingVer; }
            set { _paddingVer = value; }
        }

        [Description("Colour of the button text")]
        [Category("Appearance")]
        [DisplayName("TextColor")]
        [RefreshProperties(RefreshProperties.All)]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        [SerializeAsHex]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
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
            set { _pushedImage = Math.Max(0, value); }
        }

        [Description("Image to display when the player moves their mouse over the button")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int MouseoverImage
        {
            get { return _mouseoverImage; }
            set { _mouseoverImage = Math.Max(0, value); }
        }

        [Description("Optional image to be displayed on the button")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = Math.Max(0, value); }
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
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string Text
        {
            get { return _text; }
            set { _text = value; }
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
