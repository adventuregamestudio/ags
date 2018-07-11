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
    [DefaultProperty("BackgroundImage")]
    public abstract class GUI : IToXml, IComparable<GUI>
    {
        public const int LEGACY_MAX_CONTROLS_PER_GUI = 30;

        public GUI()
        {
            _name = string.Empty;
            _bgcol = 8;
        }

        protected string _name;
        protected int _id;
        protected int _bgcol;
        protected int _bgimage;
        protected List<GUIControl> _controls = new List<GUIControl>();

        /// <summary>
        /// Width of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public virtual int EditorWidth
        {
            get { return 0; }
        }

        /// <summary>
        /// Height of the GUI, as displayed in the Editor.
        /// </summary>
        [Browsable(false)]
        [AGSNoSerialize]
        public virtual int EditorHeight
        {
            get { return 0; }
        }

        [Description("Background color of the GUI (0 for transparent)")]
        [Category("Appearance")]
        [DisplayName("BackgroundColourNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int BackgroundColor
        {
            get { return _bgcol; }
            set { _bgcol = value; }
        }

        [Description("Background color for the GUI (0,0,0 = transparent)")]
        [Category("Appearance")]
        [DisplayName("BackgroundColour")]
        [RefreshProperties(RefreshProperties.All)]
        [AGSNoSerialize]
        public Color BackgroundColorRGB
        {
            get
            {
                return new AGSColor(_bgcol).ToRgb();
            }
            set
            {
                _bgcol = new AGSColor(value).ColorNumber;
            }
        }

        [Description("Background image for the GUI (0 for none)")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int BackgroundImage
        {
            get { return _bgimage; }
            set { _bgimage = value; }
        }

        [Description("The ID number of the GUI")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The script name of the GUI")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set
            {
                _name = Utilities.ValidateScriptName(value);
            }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "GUI: " + this.Name; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return this.Name + " (" + this.GetType().Name + " " + this.ID + ")"; }
        }

        [Browsable(false)]
        [AGSNoSerialize()]
        public List<GUIControl> Controls
        {
            get { return _controls; }
            set { _controls = value; }
        }

        public void SendControlToBack(GUIControl controlToSend)
        {
            int currentZOrder = controlToSend.ZOrder;
            foreach (GUIControl control in _controls)
            {
                if (control.ZOrder < currentZOrder)
                {
                    control.ZOrder++;
                }
            }

            controlToSend.ZOrder = 0;
        }

        public void BringControlToFront(GUIControl controlToSend)
        {
            int currentZOrder = controlToSend.ZOrder;
            foreach (GUIControl control in _controls)
            {
                if (control.ZOrder > currentZOrder)
                {
                    control.ZOrder--;
                }
            }

            controlToSend.ZOrder = _controls.Count - 1;
        }

        public void DeleteControl(GUIControl controlToDelete)
        {
            BringControlToFront(controlToDelete);
            _controls.Remove(controlToDelete);
            int currentID = controlToDelete.ID;
            foreach (GUIControl control in _controls)
            {
                if (control.ID > currentID)
                {
                    control.ID--;
                }
            }
        }

        public GUI(XmlNode rootGuiNode)
            : this()
        {
            SerializeUtils.DeserializeFromXML(this, rootGuiNode);

            _controls = new List<GUIControl>();

            foreach (XmlNode node in SerializeUtils.GetChildNodes(rootGuiNode, "Controls"))
            {
                if (node.Name == "GUIButton")
                {
                    _controls.Add(new GUIButton(node));
                }
                else if (node.Name == "GUIInventory")
                {
                    _controls.Add(new GUIInventory(node));
                }
                else if (node.Name == "GUILabel")
                {
                    _controls.Add(new GUILabel(node));
                }
                else if (node.Name == "GUIListBox")
                {
                    _controls.Add(new GUIListBox(node));
                }
                else if (node.Name == "GUISlider")
                {
                    _controls.Add(new GUISlider(node));
                }
                else if (node.Name == "GUITextBox")
                {
                    _controls.Add(new GUITextBox(node));
                }
                else if (node.Name == "GUITextWindowEdge")
                {
                    _controls.Add(new GUITextWindowEdge(node));
                }
                else
                {
                    throw new InvalidDataException("Unknown control type: " + node.Name);
                }
            }
        }

        public virtual void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);

            writer.WriteStartElement("Controls");
            foreach (GUIControl control in _controls)
            {
                control.ToXml(writer);
            }
            writer.WriteEndElement();
        }

        #region IComparable<GUI> Members

        public int CompareTo(GUI other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
