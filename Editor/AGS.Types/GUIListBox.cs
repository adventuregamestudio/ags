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
    public class GUIListBox : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "ListBox";
        public const string SCRIPT_CLASS_TYPE = "ListBox";

        public GUIListBox(int x, int y, int width, int height)
            : base(x, y, width, height)
        {
            _showBorder = true;
            _showScrollArrows = true;
            _textColor = 0;
            _selectedTextColor = 7;
            _selectedBackgroundColor = 16;
            _textAlignment = ListBoxTextAlignment.Left;
            _translated = true;
        }

        public GUIListBox(XmlNode node) : base(node)
        {
        }

        public GUIListBox() { }

        private int _font;
        private int _textColor;
        private int _selectedTextColor;
        private int _selectedBackgroundColor;
        private ListBoxTextAlignment _textAlignment;
        private bool _showBorder;
        private bool _showScrollArrows;
        private string _selectionChangedEventHandler = string.Empty;
        // We set _translated to true only when new control is created, because it won't be
        // deserialized when importing older projects, and we need it to be disabled for backward compatibility
        private bool _translated;

        [Description("Script function to run when the selection is changed")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventProperty()]
        [ScriptFunctionParameters("GUIControl *control")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnSelectionChanged
        {
            get { return _selectionChangedEventHandler; }
            set
            {
                if (value.Length > MAX_EVENT_HANDLER_LENGTH)
                {
                    _selectionChangedEventHandler = value.Substring(0, MAX_EVENT_HANDLER_LENGTH);
                }
                else
                {
                    _selectionChangedEventHandler = value;
                }
            }
        }

        [Description("Determines whether the listbox default up/down scroll arrows are drawn")]
        [Category("Appearance")]
        public bool ShowScrollArrows
        {
            get { return _showScrollArrows; }
            set { _showScrollArrows = value; }
        }

        [Description("Determines whether the listbox border is drawn")]
        [Category("Appearance")]
        public bool ShowBorder
        {
            get { return _showBorder; }
            set { _showBorder = value; }
        }

        [Description("Indentation of the text in the listbox")]
        [Category("Appearance")]
        public ListBoxTextAlignment TextAlignment
        {
            get { return _textAlignment; }
            set { _textAlignment = value; }
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
        [DisplayNameAttribute("TextColor")]
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

        [Description("AGS Colour Number of the selected item's text")]
        [Category("Appearance")]
        [DisplayName("SelectedTextColourNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int SelectedTextColor
        {
            get { return _selectedTextColor; }
            set { _selectedTextColor = value; }
        }

        [Description("Selected item's text colour")]
        [Category("Appearance")]
        [DisplayName("SelectedTextColor")]
        [RefreshProperties(RefreshProperties.All)]
        [AGSNoSerialize]
        public Color SelectedTextColorRGB
        {
            get
            {
                return new AGSColor(_selectedTextColor).ToRgb();
            }
            set
            {
                _selectedTextColor = new AGSColor(value).ColorNumber;
            }
        }

        [Description("Background colour for the selected item")]
        [Category("Appearance")]
        [DisplayName("SelectedBackgroundColorNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int SelectedBackgroundColor
        {
            get { return _selectedBackgroundColor; }
            set { _selectedBackgroundColor = value; }
        }

        [Description("Selected item's background colour")]
        [Category("Appearance")]
        [DisplayName("SelectedBackgroundColor")]
        [RefreshProperties(RefreshProperties.All)]
        [AGSNoSerialize]
        public Color SelectedBackgroundColorRGB
        {
            get
            {
                return new AGSColor(_selectedBackgroundColor).ToRgb();
            }
            set
            {
                _selectedBackgroundColor = new AGSColor(value).ColorNumber;
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

        [Description("If true, the list box items will be translated to current language")]
        [Category("Appearance")]
        public bool Translated
        {
            get { return _translated; }
            set { _translated = value; }
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
    }
}