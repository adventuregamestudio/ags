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

        private TextParserEditor _editor;
        private ContentDocument _document;

        public TextParserComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _guiController.RegisterIcon("TextParserIcon", Resources.ResourceManager.GetIcon("textparser.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Text Parser", "TextParserIcon");
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _document.Dispose();
            }
            _editor = new TextParserEditor(_agsEditor.CurrentGame.TextParser);
            _document = new ContentDocument(_editor, "Text Parser", this);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.TextParser; }
        }

        public override void CommandClick(string controlID)
        {
            _guiController.AddOrShowPane(_document);
			_guiController.ShowCuppit("Use the Text Parser if you want to have the player type commands in to the game. This is fairly advanced stuff, so I'd recommend you start off with a point-and-click interface.", "Text Parser introduction");
		}

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
        }

    }
}
