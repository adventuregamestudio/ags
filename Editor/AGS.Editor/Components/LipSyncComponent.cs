using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class LipSyncComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "LipSync";
        private const string ICON_KEY = "LipSyncIcon";

        private LipSyncEditor _editor;
        private ContentDocument _document;

        public LipSyncComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("lips.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Lip sync", ICON_KEY);
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _document.Dispose();
            }
            _editor = new LipSyncEditor(_agsEditor.CurrentGame.LipSync);
            _document = new ContentDocument(_editor, "Lip sync", this, ICON_KEY);
            _document.SelectedPropertyGridObject = _editor.EditingLipSync;
        }

        public override string ComponentID
        {
            get { return ComponentIDs.LipSync; }
        }

        public override void CommandClick(string controlID)
        {
            if (_document.Control.IsDisposed)
            {
                RecreateDocument();
            }
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
            _guiController.ShowCuppit("AGS supports two types of lip sync so that characters mouths appear to move in line with what they're saying. This is fairly advanced stuff, so I'd ignore it to start with. Read up on it in the manual later if you like.", "Lip Sync introduction");
        }

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
        }

    }
}
