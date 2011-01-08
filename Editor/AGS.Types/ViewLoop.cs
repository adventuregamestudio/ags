using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class ViewLoop
    {
        public static readonly string[] DirectionNames = new string[]{"down", "left", "right", "up", "down-right", "up-right", "down-left", "up-left"};

        private int _id;
        private bool _runNextLoop;
        private List<ViewFrame> _frames = new List<ViewFrame>();

        public ViewLoop()
        {
        }

		public ViewLoop(int id)
		{
			_id = id;
		}

        [Description("The ID number of the loop")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("Whether to run the next loop after this one to create a long animation")]
        [Category("Design")]
        public bool RunNextLoop
        {
            get { return _runNextLoop; }
            set { _runNextLoop = value; }
        }
		
		/// <summary>
		/// This is pretty much obsolete now, it used to be Full if the
		/// Max Frames Per Loop was reached, but now there isn't one.
		/// </summary>
        [Browsable(false)]
        public bool Full
        {
            get { return false; }
        }

        [Browsable(false)]
        public string DirectionDescription
        {
            get
            {
                if (_id < DirectionNames.Length)
                {
                    return DirectionNames[_id];
                }
                return "No special purpose";
            }
        }

        [Browsable(false)]
        public List<ViewFrame> Frames
        {
            get { return _frames; }
        }

        public ViewLoop(XmlNode node)
        {
            ID = Convert.ToInt32(SerializeUtils.GetElementString(node, "ID"));
            RunNextLoop = Boolean.Parse(SerializeUtils.GetElementString(node, "RunNextLoop"));
            foreach (XmlNode frameNode in SerializeUtils.GetChildNodes(node, "Frames"))
            {
                _frames.Add(new ViewFrame(frameNode));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Loop");
            writer.WriteElementString("ID", ID.ToString());
            writer.WriteElementString("RunNextLoop", _runNextLoop.ToString());

            writer.WriteStartElement("Frames");
            foreach (ViewFrame frame in _frames)
            {
                frame.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteEndElement();
        }

        public ViewLoop Clone()
        {
            return Clone(false);
        }

        public ViewLoop Clone(bool flipped)
        {
            ViewLoop clone = new ViewLoop 
            {
                _frames = new List<ViewFrame>(),
                _runNextLoop = RunNextLoop
            };

            foreach (ViewFrame frame in _frames)
            {
                clone.Frames.Add(frame.Clone(flipped));
            }

            return clone;
        }
    }
}
