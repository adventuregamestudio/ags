using System;
using System.ComponentModel;
using System.IO;
using System.Text;

namespace AGS.Types
{
    public class DialogScript
    {
        private string _fileName;
        private string _text = string.Empty;
        private bool _modified = false;
        // FIX-ME: this is not used yet but will be used by the file listener
        // remove this comment once file listener is implemented
        private bool _isBeingSaved = false;
        private DateTime _lastSavedAt = DateTime.MinValue;
        private static readonly string DEFAULT_NEW_DIALOG_SCRIPT = 
            "// Dialog script file" + Environment.NewLine +
            "@S  // Dialog startup entry point" + Environment.NewLine +
            "return" + Environment.NewLine;

        public const string DIALOGUES_DIR = "Dialogs"; // Directory
        public const string DIALOGUES_EXT = ".asd"; // Extension

        public static Encoding TextEncoding
        {  
            get { return Script.TextEncoding; }
        }

        public static string GetFileName(string dialog_name)
        {
            return Path.Combine(DIALOGUES_DIR, dialog_name + DIALOGUES_EXT);
        }

        public static string GetFileName(Dialog dialog)
        {
            return Path.Combine(DIALOGUES_DIR, dialog.Name + DIALOGUES_EXT);
        }

        public static DialogScript CreateDefault(string filename)
        {
            return new DialogScript(filename, DEFAULT_NEW_DIALOG_SCRIPT);
        }

        /// <summary>
        /// Creates a new Dialog Script which is the main part of a Dialog that is stored in a Dialog.
        /// </summary>
        /// <param name="fileName">The dialog script filename.
        /// an underscore.</param>
        /// <param name="text">The script itself.</param>
        public DialogScript(string fileName, string text)
        {
            _fileName = fileName;
            _text = text ?? string.Empty;
        }

        [Browsable(false)]
        public string Text
        {
            get { return _text; }
            set
            {
                if (_text != value)
                {
                    _text = value ?? string.Empty;
                    _modified = true;
                }
            }
        }

        [ReadOnly(true)]
        [Category("Setup")]
        [Description("File name that the dialog script is stored in")]
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; }
        }

        [Browsable(false)]
        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        [Browsable(false)]
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
