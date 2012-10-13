using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class SpriteManagerComponent : BaseComponent, ISpriteController
    {
        private const string TOP_LEVEL_COMMAND_ID = "SpriteManager";
        private const string ICON_KEY = "SpriteManagerIcon";
        
        private SpriteManager _sprEditor;
        private ContentDocument _editorPane;

        public SpriteManagerComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            Init();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("iconspr.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Sprites", ICON_KEY);
            Factory.Events.ShowSpriteManager += new EditorEvents.ShowSpriteManagerHandler(Events_ShowSpriteManager);
            RefreshDataFromGame();
        }

        private void Init()
        {
            _sprEditor = new SpriteManager();
            _editorPane = new ContentDocument(_sprEditor, "Sprites", this, ICON_KEY);            
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
            if (_editorPane.Control.IsDisposed)
            {
                Init();
                RefreshDataFromGame();
            }
            _editorPane.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_editorPane);
            _sprEditor.PopulatePropertyGrid();
			_guiController.ShowCuppit("The Sprite Manager is where you import all your graphics, except room backgrounds. Once your images are in the sprite manager, they can be used across the game. Right-click in the main Sprite Manager window to try it out.", "Sprite Manager introduction");
		}

        public void Refresh()
        {
            RefreshDataFromGame();
        }

        public override void RefreshDataFromGame()
        {
            _sprEditor.GameChanged();
        }

    }
}
