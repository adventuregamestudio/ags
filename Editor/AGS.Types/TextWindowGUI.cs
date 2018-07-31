using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("TextColor")]
    public class TextWindowGUI : GUI
    {
		private int _textColor;
		private int _padding = 3;

        /// <summary>
        /// Width of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public override int EditorWidth
        {
            get { return 200; }
        }

        /// <summary>
        /// Height of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public override int EditorHeight
        {
            get { return 100; }
        }

		[Description("Colour of the text when drawn on this text window")]
		[Category("Appearance")]
		public int TextColor
		{
			get { return _textColor; }
			set { _textColor = value; }
		}

		[Description("The amount of padding, in pixels, surrounding the text in this text window")]
		[Category("Appearance")]
		public int Padding
		{
			get { return _padding; }
			set { _padding = value; }
		}

		// For backwards compatibility, before 3.0.0.19 stored
		// it as "BorderColor"
		[Browsable(false)]
		[AGSNoSerialize]
		public int BorderColor
		{
			get { return _textColor; }
			set { _textColor = value; }
		}

        public TextWindowGUI() : base()
        {
            _controls.Add(new GUITextWindowEdge(0, 0, 0));
            _controls.Add(new GUITextWindowEdge(0, 90, 1));
            _controls.Add(new GUITextWindowEdge(90, 0, 2));
            _controls.Add(new GUITextWindowEdge(90, 90, 3));
            _controls.Add(new GUITextWindowEdge(0, 40, 4));
            _controls.Add(new GUITextWindowEdge(90, 40, 5));
            _controls.Add(new GUITextWindowEdge(40, 0, 6));
            _controls.Add(new GUITextWindowEdge(40, 90, 7));
        }
        
        public TextWindowGUI(XmlNode rootGuiNode) : base(rootGuiNode)
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
