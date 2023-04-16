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
            Factory.Events.SpritesImported += new EditorEvents.SpriteImportHandler(Events_OnSpritesImported);
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

        private void Events_OnSpritesImported(int[] spriteNumbers)
        {
            Refresh();
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
		}

        public void Refresh()
        {
            RefreshDataFromGame();
        }

        public override void RefreshDataFromGame()
        {
            Sprite.AllowRelativeResolution = Factory.AGSEditor.CurrentGame.Settings.AllowRelativeAssetResolutions;
            _sprEditor.GameChanged();
        }

        public override void GameSettingsChanged()
        {
            Sprite.AllowRelativeResolution = Factory.AGSEditor.CurrentGame.Settings.AllowRelativeAssetResolutions;
        }
    }
}
