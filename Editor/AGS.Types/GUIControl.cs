using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Types
{
    [Serializable]
    public abstract class GUIControl : ICloneable
    {
        protected GUIControl(int x, int y, int width, int height)
        {
            _name = string.Empty;
            _width = width;
            _height = height;
            _x = x;
            _y = y;
            _zorder = 0;
            _locked = false;
        }

        protected GUIControl(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        protected GUIControl() { }

        protected string _name;
        protected int _id;
        protected int _zorder;
        private int _width;
        private int _height;
        private int _x;
        private int _y;
        private bool _locked;
        private bool _clickable = true;
        private bool _enabled = true;
        private bool _visible = true;

        [AGSNoSerialize]
        private GUIControlGroup _memberOf;
        
        [Browsable(false)]
        [AGSNoSerialize]
        public GUIControlGroup MemberOf
        {
            get { return _memberOf; }
            set { _memberOf = value; }
        }

        [Description("The height, in pixels, of the control")]
        [Category("Layout")]
        public int Height
        {
            get { return _height; }
            set
            {
                if (value < 1)
                {
                    throw new ArgumentException("GUIControl Height must be greater than zero.");
                }
                _height = value;
            }
        }

        [Description("A value denoting if the control can be moved in the editor.")]
        [Category("Layout")]
        public bool Locked
        {
            get { return _locked; }
            set { _locked = value; }
        }

        [Description("The width, in pixels, of the control")]
        [Category("Layout")]
        public int Width
        {
            get { return _width; }
            set
            {
                if (value < 1)
                {
                    throw new ArgumentException("GUIControl Width must be greater than zero.");
                }
                _width = value;
            }
        }

        [Description("Left hand edge (X co-ordinate) of the control")]
        [Category("Layout")]
        public int Left
        {
            get { return _x; }
            set { _x = value; }
        }

        [Description("Top edge (Y co-ordinate) of the control")]
        [Category("Layout")]
        public int Top
        {
            get { return _y; }
            set { _y = value; }
        }

        [ReadOnly(true)]
        [Description("How the control overlaps with other controls (to change, right-click the control)")]
        [Category("Layout")]
        public int ZOrder
        {
            get { return _zorder; }
            set { _zorder = value; }
        }

        [Description("The ID number of the control")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The script name of the control")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set
            {
                _name = Utilities.ValidateScriptName(value);
            }
        }

        [Description("Determines whether the Control can be clicked on, or whether mouse clicks pass straight through it")]
        [Category("Appearance")]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

        [Description("Determines whether the Control is in enabled state at the game start. Disabled controls don't react to clicks nor mouse moving over them, and their disabled looks are determined by related game setting.")]
        [Category("Appearance")]
        public bool Enabled
        {
            get { return _enabled; }
            set { _enabled = value; }
        }

        [Description("Determines whether the Control is visible at the game start")]
        [Category("Appearance")]
        public bool Visible
        {
            get { return _visible; }
            set { _visible = value; }
        }

        [Browsable(false)]
        public abstract string ControlType { get; }
        [Browsable(false)]
        public abstract string ScriptClassType { get; }

        /// <summary>
        /// Gets a list of all sprites used by this control. Useful for
        /// exporting the control.
        /// </summary>
        public List<int> GetSpritesUsed()
        {
            List<int> spriteList = new List<int>();
            GetSpritesForControl(spriteList);
            return spriteList;
        }

        protected virtual void GetSpritesForControl(List<int> list)
        {
        }

        /// <summary>
        /// Updates sprite numbers on the control using the supplied mapping.
        /// Usually used after this control has just been imported from an external
        /// source, to link up its imported sprites.
        /// </summary>
        public virtual void UpdateSpritesWithMapping(Dictionary<int, int> spriteMapping)
        {
        }

        public virtual void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void SaveToClipboard()
        {
            DataFormats.Format format = DataFormats.GetFormat(typeof(GUIControl).FullName);

            IDataObject dataObj = new DataObject();
            dataObj.SetData(format.Name, false, this);
            Clipboard.SetDataObject(dataObj, true);
        }

        public static bool AvailableOnClipboard()
        {
            IDataObject dataObj = Clipboard.GetDataObject();
            string format = typeof(GUIControl).FullName;
            if (dataObj != null) return dataObj.GetDataPresent(format);
            else return false;

        }

        public Rectangle GetRectangle()
        {
            return new Rectangle(Left, Top, Width, Height);
        }

        public static GUIControl GetFromClipBoard()
        {
            GUIControl toreturn = null;
            IDataObject dataObj = Clipboard.GetDataObject();
            string format = typeof(GUIControl).FullName;

            if (dataObj.GetDataPresent(format))
            {
                toreturn = dataObj.GetData(format) as GUIControl;
            }

            return toreturn;
        }

        public object Clone()
        {
            return this.MemberwiseClone();
        }

        public static int CompareByTop(GUIControl x, GUIControl y)
        {
            if (x.Top < y.Top) return 1;
            else if (x.Top == y.Top) return 0;
            else return -1;
        }

        public static int CompareByLeft(GUIControl x, GUIControl y)
        {
            if (x.Left < y.Left) return 1;
            else if (x.Left == y.Left) return 0;
            else return -1;
        }
    }
}
