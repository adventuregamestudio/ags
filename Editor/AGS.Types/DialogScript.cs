using AGS.Types.Interfaces;
using System;
using System.IO;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// DialogScript class implements a script related to the particular Dialog.
    /// Dialog script is a superset of a AGS script, featuring dialog-specific command syntax.
    /// </summary>
    public class DialogScript : IScript, ISaveable
    {
        public const string DIALOG_SCRIPT_FILE_EXT = "dasc";

        private string _fileName = string.Empty;
        private string _text = string.Empty;
        private bool _modified = false;
        private ScriptAutoCompleteData _autoCompleteData = new ScriptAutoCompleteData();
        // FIXME: this field will be used when file listener is implemented
        private bool _isBeingSaved = false;
        private DateTime _lastSavedAt = DateTime.MinValue;

        public DialogScript()
        {
        }

        public DialogScript(string fileName, string text)
        {
            _fileName = fileName;
            _text = text ?? string.Empty;
        }

        /// <summary>
        /// Current global text encoding for dialog scripts.
        /// TODO: store per-script, assign from the game setting when it is changed?
        /// </summary>
        public static Encoding TextEncoding
        {
            get { return Script.TextEncoding; }
        }

        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; }
        }

        public string Text
        {
            get { return _text; }
            set
            {
                _text = value;
                _modified = true;
            }
        }

        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        public ScriptAutoCompleteData AutoCompleteData
        {
            get { return _autoCompleteData; }
        }

        public bool IsBeingSaved
        {
            get { return _isBeingSaved; }
        }

        public DateTime LastSavedAt
        {
            get { return _lastSavedAt; }
        }

        public void SaveToDisk()
        {
            SaveToDisk(false);
        }

        public void SaveToDisk(bool force)
        {
            if (_modified || force)
            {
                _isBeingSaved = true;
                try
                {
                    byte[] bytes = TextEncoding.GetBytes(_text);
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
            try
            {
                using (BinaryReader reader = new BinaryReader(File.Open(_fileName, FileMode.Open, FileAccess.Read)))
                {
                    byte[] bytes = reader.ReadBytes((int)reader.BaseStream.Length);
                    _text = TextEncoding.GetString(bytes) ?? string.Empty;
                }
            }
            catch (Exception)
            {
                // TODO: add warning? would require changes to report system
                _text = string.Empty;
            }
            _modified = false;
        }
    }
}
