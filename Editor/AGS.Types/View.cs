using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class View : IToXml, IComparable<View>
    {
        public delegate void ViewUpdatedHandler(View view);
        public event ViewUpdatedHandler ViewUpdated;

        private string _name;
        private int _id;
        private List<ViewLoop> _loops = new List<ViewLoop>();

        public View()
        {
        }

        [Description("The ID number of the view")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The script name of the view")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value); }
        }

        [Browsable(false)]
        [AGSNoSerialize()]
        public List<ViewLoop> Loops
        {
            get { return _loops; }
        }

        [Browsable(false)]
        [AGSNoSerialize()]
        public string WindowTitle
        {
            get { return "View: " + Name; }
        }

        [Browsable(false)]
        [AGSNoSerialize()]
        public string NameAndID
        {
            get { return _id.ToString() + ": " + _name; }
        }

        /// <summary>
        /// Causes the ViewUpdated event to be fired. You should call this
        /// if you modify the view and need any currently open editor View
        /// windows to update to reflect the changes.
        /// </summary>
        public void NotifyClientsOfUpdate()
        {
            if (ViewUpdated != null)
            {
                ViewUpdated(this);
            }
        }

        public ViewLoop AddNewLoop()
        {
            ViewLoop newLoop = new ViewLoop();
            newLoop.ID = _loops.Count;
            _loops.Add(newLoop);
            return newLoop;
        }

        public View(XmlNode node)
        {
            ID = Convert.ToInt32(SerializeUtils.GetElementString(node, "ID"));
            Name = SerializeUtils.GetElementString(node, "Name");
            foreach (XmlNode loopNode in SerializeUtils.GetChildNodes(node, "Loops"))
            {
                _loops.Add(new ViewLoop(loopNode));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("View");
            writer.WriteElementString("ID", ID.ToString());
            writer.WriteElementString("Name", _name);

            writer.WriteStartElement("Loops");
            foreach (ViewLoop loop in _loops)
            {
                loop.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteEndElement();
        }

        #region IComparable<View> Members

        public int CompareTo(View other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
