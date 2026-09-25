using System;
using System.ComponentModel;
using System.Xml;

namespace AGS.Types
{
    public class DialogRef : IToXml
    {
        private int _id;
        private string _scriptName;
        private Dialog _dialog;

        public int ID
        {
            get { return _id; }
            set
            {
                _id = value;
                if (_dialog != null)
                    _dialog.ID = _id;
            }
        }

        public string ScriptName
        {
            get { return _scriptName; }
            set
            {
                _scriptName = Utilities.ValidateScriptName(value);
                if (_dialog != null)
                    _dialog.ScriptName = _scriptName;
            }
        }

        [AGSNoSerialize]
        public Dialog Dialog
        {
            get { return _dialog; }
            set
            {
                _dialog = value;
                _id = _dialog.ID;
                _scriptName = _dialog.ScriptName;
            }
        }

        DialogRef()
        {
        }

        public DialogRef(Dialog dialog)
        {
            _id = dialog.ID;
            _scriptName = dialog.ScriptName;
            _dialog = dialog;
        }

        public DialogRef(XmlNode node, System.Version xmlVersion)
        {
            // Test which version we are loading. If this is an old project format,
            // then we will have to also load the full Dialog data.
            // Otherwise we just read Dialog's meta-data (identifiers).
            if (xmlVersion < Dialog.FirstNewXmlVersion)
            {
                _dialog = new Dialog(node);
                _id = _dialog.ID;
                _scriptName = _dialog.ScriptName;
            }
            else
            {
                SerializeUtils.DeserializePropertiesFromXML(this, node);
                _dialog = new Dialog();
                _dialog.ID = _id;
                _dialog.ScriptName = _scriptName;
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(typeof(Dialog).Name);
            SerializeUtils.SerializePropertiesToXML(this, writer);
            writer.WriteEndElement();
        }
    }
}
