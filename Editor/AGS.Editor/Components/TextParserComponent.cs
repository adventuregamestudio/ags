using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class TextParserComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "TextParser";
        private const string ICON_KEY = "TextParserIcon";
        
        private TextParserEditor _editor;
        private ContentDocument _document;

        public TextParserComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("textparser.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Text Parser", ICON_KEY);
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _document.Dispose();
            }
            _editor = new TextParserEditor(_agsEditor.CurrentGame.TextParser);
            _document = new ContentDocument(_editor, "Text Parser", this, ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.TextParser; }
        }

        public override void CommandClick(string controlID)
        {
            if (_document.Control.IsDisposed)
            {
                RecreateDocument();
            }
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
		}

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
        }

    }
}
