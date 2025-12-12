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
        private string _scriptModule = Script.GLOBAL_SCRIPT_FILE_NAME;

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

        [Description("Background color for the GUI (0 = transparent)")]
        [Category("Appearance")]
        [RefreshProperties(RefreshProperties.All)]
        [Editor(typeof(ColorUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        [TypeConverter(typeof(CustomColorConverter))]
        public int BackgroundColor
        {
            get { return _bgcol; }
            set { _bgcol = value; }
        }

        [Description("Background image for the GUI (0 for none)")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int BackgroundImage
        {
            get { return _bgimage; }
            set { _bgimage = Math.Max(0, value); }
        }

        [Description("The ID number of the GUI")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The script name of the GUI")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set
            {
                _name = Utilities.ValidateScriptName(value);
            }
        }

        [Description("Script module which contains this GUI's event functions")]
        [Category("(Basic)")]
        [Browsable(false)]
        [AGSEventsTabProperty()]
        [TypeConverter(typeof(ScriptListTypeConverter))]
        public string ScriptModule
        {
            get { return _scriptModule; }
            set { _scriptModule = value; }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "GUI: " + this.Name; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle(GetType().Name, _name, _id); }
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
                GUIControl control = null;
                if (node.Name == "GUIButton")
                {
                    control = new GUIButton(node);
                }
                else if (node.Name == "GUIInventory")
                {
                    control = new GUIInventory(node);
                }
                else if (node.Name == "GUILabel")
                {
                    control = new GUILabel(node);
                }
                else if (node.Name == "GUIListBox")
                {
                    control = new GUIListBox(node);
                }
                else if (node.Name == "GUISlider")
                {
                    control = new GUISlider(node);
                }
                else if (node.Name == "GUITextBox")
                {
                    control = new GUITextBox(node);
                }
                else if (node.Name == "GUITextWindowEdge")
                {
                    control = new GUITextWindowEdge(node);
                }
                else
                {
                    throw new InvalidDataException("Unknown control type: " + node.Name);
                }
                control.Parent = this;
                _controls.Add(control);
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
