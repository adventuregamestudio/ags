using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public abstract class GUIControl
    {
        public const int MAX_EVENT_HANDLER_LENGTH = 30;
        protected const int MAX_NAME_LENGTH = 25;

        protected GUIControl(int x, int y, int width, int height)
        {
            _name = string.Empty;
            _width = width;
            _height = height;
            _x = x;
            _y = y;
            _zorder = 0;
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
                _name = Utilities.ValidateScriptName(value, MAX_NAME_LENGTH);
            }
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
    }
}
