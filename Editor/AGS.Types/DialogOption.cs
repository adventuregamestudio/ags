using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Text")]
    public class DialogOption
    {
        private int _id;
        private string _text = string.Empty;
        private bool _show;
        private bool _say;

        public DialogOption()
        {
        }

        [Description("The ID number of the item")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The text to display as the option")]
        [Category("Appearance")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

        [Description("Whether this option is initially visible")]
        [Category("Appearance")]
        public bool Show
        {
            get { return _show; }
            set { _show = value; }
        }

        [Description("Whether the player will repeat the option as speech after it is chosen")]
        [Category("Behaviour")]
        public bool Say
        {
            get { return _say; }
            set { _say = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public short EntryPointOffset { get; }

        public DialogOption(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

    }
}
