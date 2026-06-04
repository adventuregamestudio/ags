using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml;
using AGS.Types.Interfaces;

namespace AGS.Types
{
    [DefaultProperty("Name")]
    public class Dialog : IScript, IToXml, IComparable<Dialog>
    {
        private int _id;
        private string _name;
        private bool _showTextParser;
        private DialogScript _script;
        private bool _scriptChangedSinceLastCompile;
        private string _cachedConvertedScript;
        private List<DialogOption> _options = new List<DialogOption>();
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Dialogs);

        public Dialog(string name)
        {
            _name = Utilities.ValidateScriptName(name);
            _script = DialogScript.CreateDefault(_name);
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
            set
            { 
                _id = value;
            }
        }

        [Description("The script name of the dialog")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set {
                _name = Utilities.ValidateScriptName(value);
                _script.FileName = DialogScript.GetFileName(_name);
            }
        }

        [Browsable(false)]
        public string FileName { get { return _script.FileName; } }

        [Browsable(false)]
        public string Text { get { return _script.Text; } }

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
            get { return _script.Text; }
            set 
            {
                if (_script.Text != value)
                {
                    _scriptChangedSinceLastCompile = true;
                }
                _script.Text = value; 
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
            // FIX-ME: we will need to figure how to look the .asd file and then if it fails look into the inner text?
            // or the reverse? Need to think on this
            String fileName = DialogScript.GetFileName(_name);

            if (File.Exists(fileName))
            {
                // read from .asd file
                _script = new DialogScript(fileName, "");
                _script.LoadFromDisk();
            }
            else if (!string.IsNullOrEmpty(scriptNode.InnerText))
            {
                // try the CData?
                _script = new DialogScript(fileName, scriptNode.InnerText);
            }
            else
            {
                // I don't think we should be here???
                _script = DialogScript.CreateDefault(fileName);
            }
            _script.FileName = fileName;

            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "DialogOptions"))
            {
                _options.Add(new DialogOption(child));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            // lets save the .asd file
            _script.SaveToDisk(true);

            writer.WriteStartElement("Dialog");
            writer.WriteElementString("ID", ID.ToString());
            writer.WriteElementString("Name", _name);
            writer.WriteElementString("ShowTextParser", _showTextParser.ToString());
            writer.WriteStartElement("Script");
            // Actually I am commenting this
            // FIX-ME: move this out because the writing will be in a file by DialogScript.
            // For now we keep this so things still work.
            // writer.WriteCData(_script.Text);
            writer.WriteCData(string.Empty);
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
