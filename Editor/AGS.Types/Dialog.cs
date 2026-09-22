using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    [DefaultProperty("ScriptName")]
    public class Dialog : IToXml, IComparable<Dialog>, ICloneable
    {
        /*
         * Dialog version history:
         * 
         * 4.00.00.33   - New XML dialog format introduced
        */
        public const string LATEST_XML_VERSION = "4.0.0.33";
        public const string FIRST_XML_VERSION = "4.0.0.33";
        public static System.Version FirstNewXmlVersion = new System.Version(FIRST_XML_VERSION);

        public const string DIALOG_FILES_DIRECTORY = "Dialogs";
        public const string DIALOG_DATA_FILE_EXT = "xml";

        private static readonly string DEFAULT_NEW_DIALOG_SCRIPT =
            $@"// Dialog script file{Environment.NewLine}" +
            $"@S  // Dialog startup entry point{Environment.NewLine}" +
            $"return{Environment.NewLine}";

        // The version this dialog was loaded from
        private System.Version _savedXmlVersion = null;
        private int _id;
        private string _scriptName;
        private bool _showTextParser;
        private DialogScript _script;
        // TODO: move cached converted script to DialogScript?
        [NonSerialized]
        private string _cachedConvertedScript;
        private List<DialogOption> _options = new List<DialogOption>();
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Dialogs);

        public Dialog()
        {
            _script = new DialogScript();
            _script.Text = DEFAULT_NEW_DIALOG_SCRIPT;
            _cachedConvertedScript = null;
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
            set
            {
                string newName = Utilities.ValidateScriptName(value);
                if (newName != _scriptName)
                {
                    _scriptName = newName;
                    _script.FileName = ScriptFileName;
                }
            }
        }

        [Obsolete]
        [Browsable(false)]
        public string Name
        {
            get { return ScriptName; }
            set { ScriptName = value; }
        }

        private string DialogFileNameBase
        {
            get
            {
                // If script name is assigned, then use the script name;
                // otherwise use numeric ID, but prefix with a '@' symbol that cannot be used in script name
                return string.IsNullOrEmpty(ScriptName) ? $"@dialog{ID}" : ScriptName;
            }
        }

        /// <summary>
        /// Tells the name of the xml file that has this dialog's data.
        /// </summary>
        [Browsable(false)]
        public string DataFileName
        {
            get { return Path.Combine(DIALOG_FILES_DIRECTORY, $"{DialogFileNameBase}.{DIALOG_DATA_FILE_EXT}"); }
        }

        /// <summary>
        /// Tells the name of the file that has dialog's script.
        /// </summary>
        [Browsable(false)]
        public string ScriptFileName
        {
            get { return Path.Combine(DIALOG_FILES_DIRECTORY, $"{DialogFileNameBase}.{DialogScript.DIALOG_SCRIPT_FILE_EXT}"); }
        }

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
        public DialogScript Script
        {
            get { return _script; }
            set { _script = value; }
        }

        // TODO: perhaps move to DialogScript?
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
            LoadFromXml(node);
        }

        /// <summary>
        /// Load Dialog from the main game document, using old-style format.
        /// The version must correspond to the game project's version.
        /// </summary>
        public Dialog(XmlNode node, System.Version gameVersion)
        {
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
            XmlNode scriptNode = node.SelectSingleNode("Script");
            if (scriptNode != null)
            {
                // Read the CDATA section
                _script = new DialogScript(ScriptFileName, scriptNode.InnerText);
            }
            else
            {
                // Create a default script placeholder (??)
                _script = new DialogScript();
                _script.Text = DEFAULT_NEW_DIALOG_SCRIPT;
            }
            _script.FileName = ScriptFileName;
            _script.Modified = false;

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
