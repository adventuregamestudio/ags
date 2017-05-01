using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class SpriteManager : EditorContentPanel
    {
        private Sprite[] _selectedSprites = null;
        private SpriteFolder.SpritesUpdatedHandler _spriteUpdateHandler;
        private SpriteFolder _attachedToFolder = null;
		private bool _pendingRefreshes = false;

        public SpriteManager()
        {
            _spriteUpdateHandler = new SpriteFolder.SpritesUpdatedHandler(RootSpriteFolder_SpritesUpdated);
            InitializeComponent();
            spriteSelector.OnSelectionChanged += new SpriteSelector.SelectionChangedHandler(spriteSelector_OnSelectionChanged);

            _attachedToFolder = Factory.AGSEditor.CurrentGame.RootSpriteFolder;
            _attachedToFolder.SpritesUpdated += _spriteUpdateHandler;
        }

        protected override string OnGetHelpKeyword()
        {
            return "Importing your own sprite graphics";
        }

		protected override void OnWindowActivated()
		{
			if (_pendingRefreshes)
			{
				RefreshSprites();
				_pendingRefreshes = false;
			}
		}

		private void RefreshSprites()
		{
			spriteSelector.SetDataSource(_attachedToFolder);
		}

		private void spriteSelector_OnSelectionChanged(Sprite[] newSelection)
        {
            _selectedSprites = newSelection;

            if (newSelection == null)
            {
                return;
            }
            else if (newSelection.Length == 1)
            {
                Factory.GUIController.SetPropertyGridObject(newSelection[0]);
            }
            else if (newSelection.Length == 0)
            {
                //If there is no selection in the sprite manager, it doesn't mean there wasn't anything else selected
                //in another component, so we shouldn't clear the selection if selected item is not a sprite.
                //Especially the scenario in which a sprite is selected for a room object:
                //the sprite selector will close and refresh itself with an empty selection,
                //but if we put an empty selection in the grid, the sprite will not be updated at all,
                //the grid update operation will be cancelled.
                if (Factory.GUIController.GetPropertyGridObject() is Sprite)
                {
                    Factory.GUIController.SetPropertyGridObject(null);
                }
            }
            else
            {
                Factory.GUIController.SetPropertyGridObjects(newSelection);
            }
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            /*
            if (propertyName == Sprite.PROPERTY_RESOLUTION)
            {
                Factory.NativeProxy.SpriteResolutionsChanged(_selectedSprites);
            }
            */
        }

		protected override void OnKeyPressed(Keys keyData)
		{
			if (keyData == Keys.Delete)
			{
				spriteSelector.DeleteKeyPressed();
			}
		}

        public void PopulatePropertyGrid()
        {
            spriteSelector_OnSelectionChanged(_selectedSprites);
        }

        public bool SetSelectedSprite(int spriteNumber)
        {
            if (spriteSelector.OpenFolderForSprite(spriteNumber))
            {
                return true;
            }
            return false;
        }

        public void GameChanged()
        {
            if (_attachedToFolder != null) 
            {
                _attachedToFolder.SpritesUpdated -= _spriteUpdateHandler;
            }
            _attachedToFolder = Factory.AGSEditor.CurrentGame.RootSpriteFolder;
            _attachedToFolder.SpritesUpdated += _spriteUpdateHandler;

            spriteSelector.SetDataSource(_attachedToFolder);
        }

        private void RootSpriteFolder_SpritesUpdated()
        {
			// Refreshing the sprite list is quite slow, so only do
			// it if this pane is active
			if ((Factory.GUIController.ActivePane != null) &&
				(Factory.GUIController.ActivePane.Control == this))
			{
				RefreshSprites();
			}
			else
			{
				_pendingRefreshes = true;
			}
        }
    }
}
