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
    public class GUIButton : GUIControl, ICustomTypeDescriptor
    {
        public const string CONTROL_DISPLAY_NAME = "Button";
        public const string SCRIPT_CLASS_TYPE = "Button";

        public GUIButton(int x, int y, int width, int height) : base(x, y, width, height)
        {
            _text = "New Button";
            _textAlign = FrameAlignment.TopCenter;
            ShowBorder = true;
            SolidBackground = true;
            PaddingX = 1;
            PaddingY = 1;
            BackgroundColor = 7;
            BorderColor = 15;
            BorderShadeColor = 8;
            _clickAction = GUIClickAction.RunScript;
        }

        public GUIButton(XmlNode node) : base(node)
        {
        }

        public GUIButton() : base()
        {
        }

        private string _text = string.Empty;
        private int _image;
        private int _mouseoverImage;
        private int _pushedImage;
        private int _font;
        private ButtonColorStyle _colorStyle = ButtonColorStyle.Default;
        private int _borderShadeColor;
        private int _textColor;
        private int _textOutlineColor;
        private int _mouseoverTextColor;
        private int _pushedTextColor;
        private int _mouseoverBackgroundColor;
        private int _pushedBackgroundColor;
        private int _mouseoverBorderColor;
        private int _pushedBorderColor;
        private FrameAlignment _textAlign = FrameAlignment.TopLeft;
        private bool _wrapText;
        private bool _clipImage;
        private GUIClickAction _clickAction;
        private int _newModeNumber;
        private string _clickEventHandler = string.Empty;

        [Description("Script function to run when the button is clicked")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty(), AGSDefaultEventProperty()]
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

        [Obsolete]
        [Browsable(false)]
        public int TextPaddingHorizontal
        {
            get { return PaddingX; }
            set { PaddingX = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public int TextPaddingVertical
        {
            get { return PaddingY; }
            set { PaddingY = value; }
        }

        [Description("Which style to use for coloring this button")]
        [Category("Appearance")]
        [DefaultValue(ButtonColorStyle.Default)]
        [RefreshProperties(RefreshProperties.All)]
        [TypeConverter(typeof(EnumTypeConverter))]
        public ButtonColorStyle ColorStyle
        {
            get { return _colorStyle; }
            set { _colorStyle = value; }
        }

        [Description("Colour of the button's border shadow")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int BorderShadeColor
        {
            get { return _borderShadeColor; }
            set { _borderShadeColor = value; }
        }

        [Description("Colour of the button text")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int TextColor
        {
            get { return _textColor; }
            set { _textColor = value; }
        }

        [Description("Colour of the button text's outline (applied if the font has one)")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int TextOutlineColor
        {
            get { return _textOutlineColor; }
            set { _textOutlineColor = value; }
        }

        [Description("Colour of the button's background when the player moves their mouse over the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int MouseOverBackgroundColor
        {
            get { return _mouseoverBackgroundColor; }
            set { _mouseoverBackgroundColor = value; }
        }

        [Description("Colour of the button's background when the player clicks the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int PushedBackgroundColor
        {
            get { return _pushedBackgroundColor; }
            set { _pushedBackgroundColor = value; }
        }

        [Description("Colour of the button's border when the player moves their mouse over the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int MouseOverBorderColor
        {
            get { return _mouseoverBorderColor; }
            set { _mouseoverBorderColor = value; }
        }

        [Description("Colour of the button's border when the player clicks the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int PushedBorderColor
        {
            get { return _pushedBorderColor; }
            set { _pushedBorderColor = value; }
        }

        [Description("Colour of the button text when the player moves their mouse over the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int MouseOverTextColor
        {
            get { return _mouseoverTextColor; }
            set { _mouseoverTextColor = value; }
        }

        [Description("Colour of the button text when the player clicks the button")]
        [Category("Appearance")]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int PushedTextColor
        {
            get { return _pushedTextColor; }
            set { _pushedTextColor = value; }
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

#region ICustomTypeDescriptor Members

        public AttributeCollection GetAttributes()
        {
            return TypeDescriptor.GetAttributes(this, true);
        }

        public string GetClassName()
        {
            return TypeDescriptor.GetClassName(this, true);
        }

        public string GetComponentName()
        {
            return TypeDescriptor.GetComponentName(this, true);
        }

        public TypeConverter GetConverter()
        {
            return TypeDescriptor.GetConverter(this, true);
        }

        public EventDescriptor GetDefaultEvent()
        {
            return TypeDescriptor.GetDefaultEvent(this, true);
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return TypeDescriptor.GetDefaultProperty(this, true);
        }

        public object GetEditor(Type editorBaseType)
        {
            return TypeDescriptor.GetEditor(this, editorBaseType, true);
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            // Hide certain color state properties, depending on the "ColorStyle" property value
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if (_colorStyle == ButtonColorStyle.Default)
                {
                    if ((property.Name == "MouseOverBackgroundColor") || (property.Name == "PushedBackgroundColor") ||
                        (property.Name == "MouseOverBorderColor") || (property.Name == "PushedBorderColor") ||
                        (property.Name == "MouseOverTextColor") || (property.Name == "PushedTextColor"))
                    {
                        wantThisProperty = false;
                    }
                }
                else if (_colorStyle == ButtonColorStyle.Dynamic)
                {
                    if ((property.Name == "MouseOverBorderColor") || (property.Name == "PushedBorderColor"))
                    {
                        wantThisProperty = false;
                    }
                }

                if (wantThisProperty)
                {
                    wantProperties.Add(property);
                }
            }
            return new PropertyDescriptorCollection(wantProperties.ToArray());
        }

        public PropertyDescriptorCollection GetProperties()
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, true);
            return properties;
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }

        #endregion
    }
}