using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class NormalGUI : GUI
    {
        public const int MAX_EVENT_HANDLER_LENGTH = 19;
        public const string XML_ELEMENT_NAME = "NormalGUI";

        public NormalGUI() : base()
        {
            _width = 300;
            _height = 200;
            _x = 0;
            _y = 0;
			_bordercol = 2;
		}

        private int _width;
        private int _height;
        private int _x;
        private int _y;
        private bool _clickable = true;
        private GUIVisibility _visibility;
        private int _popupYPos;
        private int _zorder;
		private int _bordercol;
        private int _transparency = 0;
		private string _clickEventHandler = string.Empty;

        [Description("Script function to run when the GUI is clicked")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventProperty()]
        [ScriptFunctionParameters("GUI *theGui, MouseButton button")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnClick
        {
            get { return _clickEventHandler; }
            set
            {
                if (value.Length > MAX_EVENT_HANDLER_LENGTH)
                {
                    _clickEventHandler = value.Substring(0, MAX_EVENT_HANDLER_LENGTH);
                }
                else
                {
                    _clickEventHandler = value;
                }
            }
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

        [Description("Determines when the GUI will be visible on the screen")]
        [Category("Appearance")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GUIVisibility Visibility
        {
            get { return _visibility; }
            set { _visibility = value; }
        }

        [Description("The Y co-ordinate at which the GUI will appear when using MouseYPos visibility")]
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
