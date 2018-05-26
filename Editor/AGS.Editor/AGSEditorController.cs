using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
	/// <summary>
	/// Used by plugins to access AGS Editor features
	/// </summary>
	public class AGSEditorController : IAGSEditor
	{
		private AGSEditor _agsEditor;
		private GUIController _guiController;
		private ComponentController _componentController;
		private IRoomController _roomController = null;

		public AGSEditorController(ComponentController componentController, AGSEditor agsEditor, GUIController guiController)
		{
			_componentController = componentController;
			_agsEditor = agsEditor;
			_guiController = guiController;
		}

		void IAGSEditor.AddComponent(IEditorComponent component)
		{
			_componentController.AddComponent(component);
		}

        IList<IEditorComponent> IAGSEditor.Components
        {
            get
            {
                return _componentController.Components;
            }
        }

		IGUIController IAGSEditor.GUIController
		{
			get { return _guiController; }
		}

		IGame IAGSEditor.CurrentGame
		{
			get { return _agsEditor.CurrentGame; }
		}

		string IAGSEditor.Version
		{
			get { return AGS.Types.Version.AGS_EDITOR_VERSION; }
		}

		event GetScriptHeaderListHandler IAGSEditor.GetScriptHeaderList
		{
			add { _agsEditor.GetScriptHeaderList += value; }
			remove { _agsEditor.GetScriptHeaderList -= value; }
		}

		void IAGSEditor.RebuildAutocompleteCache(Script script)
		{
			AutoComplete.ConstructCache(script);
		}

		IList<Script> IAGSEditor.GetAllScriptHeaders()
		{
			return _agsEditor.GetAllScriptHeaders();
		}

		Bitmap IAGSEditor.GetSpriteImage(int spriteNumber)
		{
			return Factory.NativeProxy.GetBitmapForSprite(spriteNumber);
		}

        void IAGSEditor.ChangeSpriteImage(int spriteNumber, Bitmap newImage, SpriteImportTransparency transparencyType, bool useAlphaChannel)
        {
            Sprite sprite = _agsEditor.CurrentGame.RootSpriteFolder.FindSpriteByID(spriteNumber, true);
            if (sprite == null)
            {
                throw new AGSEditorException("Unable to find sprite " + spriteNumber + " in any sprite folders");
            }

            Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, newImage, (SpriteImportMethod)((int)transparencyType), true, false, useAlphaChannel);
            if (sprite.ColorDepth < 32)
            {
                sprite.AlphaChannel = false;
            }
            sprite.SourceFile = string.Empty;
            _agsEditor.CurrentGame.RootSpriteFolder.NotifyClientsOfUpdate();
        }

        Sprite IAGSEditor.CreateNewSprite(ISpriteFolder inFolder, Bitmap newImage, SpriteImportTransparency transparencyType, bool useAlphaChannel)
        {
            Sprite newSprite = Factory.NativeProxy.CreateSpriteFromBitmap(newImage, (SpriteImportMethod)((int)transparencyType), true, false, useAlphaChannel);
            if (newSprite.ColorDepth < 32)
            {
                newSprite.AlphaChannel = false;
            }
            newSprite.SourceFile = string.Empty;
            inFolder.Sprites.Add(newSprite);
            _agsEditor.CurrentGame.RootSpriteFolder.NotifyClientsOfUpdate();
            return newSprite;
        }

        string IAGSEditor.GetSpriteUsageReport(int spriteNumber)
        {
            return new SpriteUsageChecker().GetSpriteUsageReport(spriteNumber, _agsEditor.CurrentGame);
        }

        void IAGSEditor.DeleteSprite(int spriteNumber)
        {
            Sprite sprite = _agsEditor.CurrentGame.RootSpriteFolder.FindSpriteByID(spriteNumber, true);
            if (sprite == null)
            {
                throw new AGSEditorException("The sprite " + spriteNumber + " could not be found");
            }
            _agsEditor.DeleteSprite(sprite);
            _agsEditor.CurrentGame.RootSpriteFolder.NotifyClientsOfUpdate();
        }

		ISourceControlIntegration IAGSEditor.SourceControl
		{
			get { return _agsEditor; }
		}

		IRoomController IAGSEditor.RoomController
		{
			get
			{
				if (_roomController == null)
				{
					_roomController = (IRoomController)_componentController.FindComponentThatImplementsInterface(typeof(IRoomController));
				}
				return _roomController;
			}
		}

        ISourceControlProvider IAGSEditor.SourceControlProvider
        {
            set
            {
                _agsEditor.SourceControlProvider = value;
            }
        }
	}
}
