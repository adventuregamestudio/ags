using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using AGS.Types.Interfaces;

namespace AGS.Types
{
    [Serializable]
    [DefaultProperty("ScriptName")]
    public class Dialog : IScript, IToXml, IComparable<Dialog>, ICloneable
    {
        /*
         * Dialog version history:
         * 
         * 4.00.00.33   - New XML dialog format introduced
        */
        public const string LATEST_XML_VERSION = "4.0.0.33";
        private const string FIRST_XML_VERSION = "4.0.0.33";

        private const string DIALOG_DATA_FILE_EXT = "xml";

        // The version this dialog was loaded from
        private System.Version _savedXmlVersion = null;
        private int _id;
        private string _scriptName;
        private bool _showTextParser;
        private string _script;
        [NonSerialized]
        private bool _scriptChangedSinceLastCompile;
        [NonSerialized]
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
        public string ScriptName
        {
            get { return _scriptName; }
            set { _scriptName = Utilities.ValidateScriptName(value); }
        }

        [Obsolete]
        [Browsable(false)]
        public string Name
        {
            get { return ScriptName; }
            set { ScriptName = value; }
        }

        /// <summary>
        /// Tells the name of the xml file that has this dialog's data.
        /// </summary>
        [Browsable(false)]
        public string DataFileName
        {
            get
            {
                return string.IsNullOrEmpty(ScriptName) ?
                    $"@dialog{ID}.{DIALOG_DATA_FILE_EXT}" :
                    $"{ScriptName}.{DIALOG_DATA_FILE_EXT}";
            }
        }

        // This is IScript.Filename impl, need to be removed
        // after Dialog is no longer implementing IScript
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
            get { return string.IsNullOrEmpty(this.ScriptName) ? ("Dialog " + this.ID) : ("Dialog: " + this.ScriptName); }
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
            get { return TypesHelper.MakePropertyGridTitle("Dialog", _scriptName, _id); }
        }

        public Dialog(XmlNode node)
        {
            _scriptChangedSinceLastCompile = true;
            LoadFromXml(node);
        }

        /// <summary>
        /// Load Dialog from the main game document, using old-style format.
        /// The version must correspond to the game project's version.
        /// </summary>
        public Dialog(XmlNode node, System.Version gameVersion)
        {
            _scriptChangedSinceLastCompile = true;
            LoadDialogContents(node, gameVersion);
        }

        public void LoadFromXml(XmlNode node)
        {
            // First of all, test which format version are we loading
            System.Version fileVersion;
            try
            {
                fileVersion = SerializeUtils.ReadVersionAttribute(node);
            }
            catch (Exception)
            {
                throw new AGSEditorException("Dialog data file has an invalid version identifier.");
            }

            _scriptChangedSinceLastCompile = true;
            LoadDialogContents(node, fileVersion);
        }

        /// <summary>
        /// Loads Dialog contents from XML.
        /// </summary>
        private void LoadDialogContents(XmlNode node, System.Version xmlVersion)
        {
            // Since the new Dialog serialization format we should expect that
            // the xml contains only dialog identification (id, scriptname).
            // Only these two fields are obligatory (actually, scriptname is
            // still optional due to current AGS rules). For the rest we must
            // expect that they do not exist in the current document.
            _id = Convert.ToInt32(SerializeUtils.GetElementString(node, "ID"));
            // old-style script name (will get overridden by new one
            _scriptName = SerializeUtils.GetElementStringOrDefault(node, "Name", _scriptName);
            _scriptName = SerializeUtils.GetElementStringOrDefault(node, "ScriptName", _scriptName);
            _showTextParser = Boolean.Parse(SerializeUtils.GetElementStringOrDefault(node, "ShowTextParser", bool.FalseString));

            // Script node may or may not exist (it may be also external file)
            _script = null;
            XmlNode scriptNode = node.SelectSingleNode("Script");
            if (scriptNode != null)
            {
                // Luckily the CDATA section is easy to read back
                _script = scriptNode.InnerText;
            }

            _options.Clear();
            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "DialogOptions"))
            {
                _options.Add(new DialogOption(child));
            }

            _savedXmlVersion = xmlVersion;
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(GetType().Name);
            writer.WriteAttributeString("Version", LATEST_XML_VERSION);
            writer.WriteElementString("ID", ID.ToString());
            writer.WriteElementString("ScriptName", _scriptName);
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

        #region IClonable Members

        public object Clone()
        {
            Dialog copy = this.MemberwiseClone() as Dialog;
            copy._options = new List<DialogOption>();
            foreach (var option in this._options)
                copy._options.Add(option.Clone() as DialogOption);
            return copy;
        }

        #endregion
    }
}
