using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class SpriteManagerComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "SpriteManager";

        private SpriteManager _sprEditor;
        private ContentDocument _editorPane;

        public SpriteManagerComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _sprEditor = new SpriteManager();
            _editorPane = new ContentDocument(_sprEditor, "Sprites", this);
            _guiController.RegisterIcon("SpriteManagerIcon", Resources.ResourceManager.GetIcon("iconspr.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Sprites", "SpriteManagerIcon");
            Factory.Events.ShowSpriteManager += new EditorEvents.ShowSpriteManagerHandler(Events_ShowSpriteManager);
            RefreshDataFromGame();
        }

        private void Events_ShowSpriteManager(int spriteNumber, ref bool successful)
        {
            CommandClick(TOP_LEVEL_COMMAND_ID);
            if (_sprEditor.SetSelectedSprite(spriteNumber))
            {
                successful = true;
            }
        }

        public override string ComponentID
        {
            get { return ComponentIDs.SpriteManager; }
        }

        public override void CommandClick(string controlID)
        {
            _guiController.AddOrShowPane(_editorPane);
            _sprEditor.PopulatePropertyGrid();
			_guiController.ShowCuppit("The Sprite Manager is where you import all your graphics, except room backgrounds. Once your images are in the sprite manager, they can be used across the game. Right-click in the main Sprite Manager window to try it out.", "Sprite Manager introduction");
		}

        public override void RefreshDataFromGame()
        {
            _sprEditor.GameChanged();
        }

    }
}
