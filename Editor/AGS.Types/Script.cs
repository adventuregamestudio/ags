using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml;
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;

namespace AGS.Types
{
    public class Script : IScript, IToXml
    {
        public const string GLOBAL_SCRIPT_FILE_NAME = "GlobalScript.asc";
        public const string GLOBAL_HEADER_FILE_NAME = "GlobalScript.ash";
        public const string CURRENT_ROOM_SCRIPT_FILE_NAME = "___CurrentRoomScript__.asc";
        public const string DIALOG_SCRIPTS_FILE_NAME = "__DialogScripts.asc";

        private string _fileName;
        private string _text;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private string _author = string.Empty;
        private string _version = string.Empty;
        private bool _isHeader = false;
        private int _uniqueKey = 0;
        private ScriptAutoCompleteData _autoCompleteData = new ScriptAutoCompleteData();
        private ICompiledScript _compiledData = null;
        private bool _modified = false;
        private bool _isBeingSaved = false;
        private int[] _breakpointedLines = new int[0];
        private DateTime _lastSavedAt = DateTime.MinValue;

		/// <summary>
		/// Creates a new Script which can be compiled with the AGS Script Compiler.
		/// </summary>
		/// <param name="fileName">The script filename. If the script is internally
		/// generated and not stored on disk, make up a name and prefix it with
		/// an underscore.</param>
		/// <param name="text">The script itself.</param>
		/// <param name="isHeader">Is this a script header or an actual script?</param>
        public Script(string fileName, string text, bool isHeader)
        {
            _fileName = fileName;
            _text = text;
            _uniqueKey = new Random().Next(Int32.MaxValue);
            _isHeader = isHeader;
        }

        public Script(string fileName, string text, string name, string description, string author, string version, int uniqueKey, bool isHeader) : this(fileName, text, isHeader)
        {
            _name = name;
            _description = description;
            _author = author;
            _version = version;
            _uniqueKey = uniqueKey;
        }

        [Browsable(false)]
        public string Text
        {
            get { return _text; }
            set { _text = value; _modified = true; }
        }

        [ReadOnly(true)]
        [Category("Setup")]
        [Description("File name that the script is stored in")]
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; }
        }

        [Category("Module information")]
        [Description("Friendly name of this script")]
        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        [Category("Module information")]
        [Description("Description of what this script does")]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

        [Category("Module information")]
        [Description("Who wrote this script?")]
        public string Author
        {
            get { return _author; }
            set { _author = value; }
        }

        [Category("Module information")]
        [Description("Version of this script")]
        public string Version
        {
            get { return _version; }
            set { _version = value; }
        }

        [Browsable(false)]
        public string NameForLabelEdit
        {
            get { return Path.GetFileNameWithoutExtension(_fileName); }
            set { _fileName = value + Path.GetExtension(_fileName); }
        }

        [Browsable(false)]
        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        [Browsable(false)]
        public bool IsBeingSaved
        {
            get { return _isBeingSaved; }
        }

        [Browsable(false)]
        public bool IsHeader
        {
            get { return _isHeader; } 
        }

        [Browsable(false)]
        public int[] BreakpointedLines
        {
            get { return _breakpointedLines; }
            set { _breakpointedLines = value; }
        }

        [Browsable(false)]
        public int UniqueKey
        {
            get { return _uniqueKey; }
        }

        [Browsable(false)]
        public ICompiledScript CompiledData
        {
            get { return _compiledData; }
            set { _compiledData = value; }
        }

        [Browsable(false)]
        public ScriptAutoCompleteData AutoCompleteData
        {
            get { return _autoCompleteData; }
            set { _autoCompleteData = value; }
        }

        [Browsable(false)]
        public DateTime LastSavedAt
        {
            get { return _lastSavedAt; }
        }

        public void SaveToDisk()
        {
            if (_modified)
            {
                _isBeingSaved = true;
                try
                {
                    // Ensure that the file gets written in 8-bit ANSI
                    byte[] bytes = Encoding.Default.GetBytes(_text);
                    using (BinaryWriter binWriter = new BinaryWriter(File.Open(_fileName, FileMode.Create)))
                    {
                        binWriter.Write(bytes);
                        _lastSavedAt = DateTime.Now;
                    }
                }
                finally
                {
                    _isBeingSaved = false;
                }
                _modified = false;
            }
        }

        public void LoadFromDisk()
        {
            // Ensure that the file gets read in 8-bit ANSI
            using (BinaryReader reader = new BinaryReader(File.Open(_fileName, FileMode.Open, FileAccess.Read)))
            {
                byte[] bytes = reader.ReadBytes((int)reader.BaseStream.Length);
                _text = Encoding.Default.GetString(bytes);
            }
			_modified = false;
        }

        public Script(XmlNode node)
        {
            if (node.Name != "Script")
            {
                throw new AGS.Types.InvalidDataException("Script node incorrect");
            }
            _fileName = SerializeUtils.GetElementString(node, "FileName");
            _name = SerializeUtils.GetElementString(node, "Name");
            _description = SerializeUtils.GetElementString(node, "Description");
            _author = SerializeUtils.GetElementString(node, "Author");
            _version = SerializeUtils.GetElementString(node, "Version");
            _uniqueKey = Convert.ToInt32(SerializeUtils.GetElementString(node, "Key"));
            _isHeader = Convert.ToBoolean(SerializeUtils.GetElementString(node, "IsHeader"));

            LoadFromDisk();

            _modified = false;
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Script");
            writer.WriteElementString("FileName", _fileName);
            writer.WriteElementString("Name", _name);
            writer.WriteElementString("Description", _description);
            writer.WriteElementString("Author", _author);
            writer.WriteElementString("Version", _version);
            writer.WriteElementString("Key", _uniqueKey.ToString());
            writer.WriteElementString("IsHeader", _isHeader.ToString());
            writer.WriteEndElement();

            SaveToDisk();
        }
    }
}
