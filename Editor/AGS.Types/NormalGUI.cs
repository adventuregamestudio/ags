using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;

namespace AGS.Types
{
    public class NormalGUI : GUI
    {
        public const string XML_ELEMENT_NAME = "NormalGUI";

        public NormalGUI(int width, int height) : base()
        {
            _width = width;
            _height = height;
            _x = 0;
            _y = 0;
            _bordercol = 2;
        }

        private int _width;
        private int _height;
        private int _x;
        private int _y;
        private bool _clickable = true;
        private bool _visible = true;
        private GUIPopupStyle _popupStyle;
        private int _popupYPos;
        private int _zorder;
		private int _bordercol;
        private int _transparency = 0;
		private string _onClick = string.Empty;

        /// <summary>
        /// Width of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public override int EditorWidth
        {
            get { return Width; }
        }

        /// <summary>
        /// Height of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public override int EditorHeight
        {
            get { return Height; }
        }

        [Description("Script function to run when the GUI is clicked")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty(), AGSDefaultEventProperty()]
        [ScriptFunction("GUI *theGui, MouseButton button")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnClick
        {
            get { return _onClick; }
            set { _onClick = value; }
        }

        [Description("The height, in pixels, of the GUI")]
        [Category("Layout")]
        public int Height
        {
            get { return _height; }
            set 
            {
                if (value < 1)
                {
                    throw new ArgumentException("GUI Height must be greater than zero.");
                }
                _height = value; 
            }
        }

        [Description("The width, in pixels, of the GUI")]
        [Category("Layout")]
        public int Width
        {
            get { return _width; }
            set
            {
                if (value < 1)
                {
                    throw new ArgumentException("GUI Width must be greater than zero.");
                }
                _width = value;
            }
        }

        [Description("Left hand edge (X co-ordinate) of the GUI")]
        [Category("Layout")]
        public int Left
        {
            get { return _x; }
            set { _x = value; }
        }

        [Description("Top edge (Y co-ordinate) of the GUI")]
        [Category("Layout")]
        public int Top
        {
            get { return _y; }
            set { _y = value; }
        }

        [Description("Determines whether the GUI can be clicked on, or whether mouse clicks pass straight through it")]
        [Category("Appearance")]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

        [Description("Determines whether the GUI is visible at the game start")]
        [Category("Appearance")]
        public bool Visible
        {
            get { return _visible; }
            set { _visible = value; }
        }

        [Description("Determines how the GUI will behave on screen")]
        [Category("Appearance")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GUIPopupStyle PopupStyle
        {
            get { return _popupStyle; }
            set { _popupStyle = value; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        [Obsolete("NormalGUI.Visibility has been replaced by NormalGUI.PopupStyle.")]
        public GUIVisibility Visibility
        {
            get { throw new NotImplementedException("Reading GUI.Visibility is no longer supported"); }
            set
            {
                if (value == GUIVisibility.NormalButInitiallyOff)
                    _popupStyle = GUIPopupStyle.Normal;
                else
                    _popupStyle = (GUIPopupStyle)value;
                Visible = value != GUIVisibility.PopupModal && value != GUIVisibility.NormalButInitiallyOff;
            }
        }

        [Description("The Y co-ordinate at which the GUI will appear when using MouseYPos popup style")]
        [Category("Appearance")]
        public int PopupYPos
        {
            get { return _popupYPos; }
            set { _popupYPos = value; }
        }

        [Description("The front-to-back z-order of this GUI relative to the other GUIs")]
        [Category("Layout")]
        public int ZOrder
        {
            get { return _zorder; }
            set { _zorder = value; }
        }

		[Description("Colour of the GUI border")]
		[Category("Appearance")]
        [DisplayName("BorderColor")]
        [RefreshProperties(RefreshProperties.All)]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        [SerializeAsHex]
        public int BorderColor
		{
			get { return _bordercol; }
			set { _bordercol = value; }
		}

        [Description("Transparency of the GUI, from 0% (solid) to 100% (invisible). Does not work with 8-bit colour games.")]
        [Category("Appearance")]
        public int Transparency
        {
            get { return _transparency; }
            set
            {
                if ((value < 0) || (value > 100))
                {
                    throw new ArgumentException("Transparency must be 0-100%");
                }
                _transparency = value;
            }
        }

        public NormalGUI(XmlNode rootGuiNode) : base(rootGuiNode)
        {
        }

        public override void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("GUIMain");

            base.ToXml(writer);

            writer.WriteEndElement();
        }
    }
}
