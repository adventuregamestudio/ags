using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using AGS.Types.Interfaces;

namespace AGS.Types
{
    [DefaultProperty("Name")]
    public class Dialog : IScript, IToXml, IComparable<Dialog>
    {
        public const int MAX_OPTIONS_PER_DIALOG = 30;

        private int _id;
        private string _name;
        private bool _showTextParser;
        private string _script;
        private bool _scriptChangedSinceLastCompile;
        private string _cachedConvertedScript;
        private List<DialogOption> _options = new List<DialogOption>();
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Dialogs);

        public Dialog()
        {
            _script = "// Dialog script file" + Environment.NewLine + 
                "@S  // Dialog startup entry point" + Environment.NewLine +
                "return" + Environment.NewLine;
            _cachedConvertedScript = null;
            _scriptChangedSinceLastCompile = true;
        }

        [Description("The ID number of the dialog")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The script name of the dialog")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value); }
        }

        [Browsable(false)]
        public string FileName { get { return "Dialog " + ID; } }

        [Browsable(false)]
        public string Text { get { return _script; } }

        [Browsable(false)]
        public ScriptAutoCompleteData AutoCompleteData { get { return null; } }

        [Description("Whether to show a text box along with the options so that the user can type in custom text")]
        [Category("Appearance")]
        public bool ShowTextParser
        {
            get { return _showTextParser; }
            set { _showTextParser = value; }
        }

        [Browsable(false)]
        public string Script
        {
            get { return _script; }
            set 
            {
                if (_script != value)
                {
                    _scriptChangedSinceLastCompile = true;
                }
                _script = value; 
            }
        }

        [Browsable(false)]
        [AGSNoSerialize]
        public bool ScriptChangedSinceLastConverted
        {
            get { return _scriptChangedSinceLastCompile; }
            set { _scriptChangedSinceLastCompile = value; }
        }

        [Browsable(false)]
        public string CachedConvertedScript
        {
            get { return _cachedConvertedScript; }
            set { _cachedConvertedScript = value; }
        }

        [Browsable(false)]
        public List<DialogOption> Options
        {
            get { return _options; }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return string.IsNullOrEmpty(this.Name) ? ("Dialog " + this.ID) : ("Dialog: " + this.Name); }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this dialog")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Dialogs;
            }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Dialog", _name, _id); }
        }

        public Dialog(XmlNode node)
        {
            _scriptChangedSinceLastCompile = true;
            _id = Convert.ToInt32(SerializeUtils.GetElementString(node, "ID"));
            _name = SerializeUtils.GetElementString(node, "Name");
            _showTextParser = Boolean.Parse(SerializeUtils.GetElementString(node, "ShowTextParser"));
            XmlNode scriptNode = node.SelectSingleNode("Script");
            // Luckily the CDATA section is easy to read back
            _script = scriptNode.InnerText;

            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "DialogOptions"))
            {
                _options.Add(new DialogOption(child));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Dialog");
            writer.WriteElementString("ID", ID.ToString());
            writer.WriteElementString("Name", _name);
            writer.WriteElementString("ShowTextParser", _showTextParser.ToString());
            writer.WriteStartElement("Script");
            writer.WriteCData(_script);
            writer.WriteEndElement();

            writer.WriteStartElement("DialogOptions");
            foreach (DialogOption option in _options)
            {
                option.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteEndElement();

        }

        #region IComparable<Dialog> Members

        public int CompareTo(Dialog other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
