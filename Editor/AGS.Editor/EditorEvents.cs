using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Editor
{
	internal class EditorEvents
	{
		public delegate void ParameterlessDelegate();
		public event ParameterlessDelegate GameSettingsChanged;
		public event ParameterlessDelegate ImportedOldGame;
		public event ParameterlessDelegate RefreshAllComponentsFromGame;
		public delegate void GameLoadHandler(XmlNode rootNode);
		public event GameLoadHandler GameLoad;
		public delegate void SavingGameHandler(XmlTextWriter writer);
		public event SavingGameHandler SavingGame;
        public delegate void SavingUserDataHandler(XmlTextWriter writer);
        public event SavingUserDataHandler SavingUserData;
        public delegate void LoadedUserDataHandler(XmlNode rootNode);
        public event LoadedUserDataHandler LoadedUserData;
        public delegate void GetAboutDialogTextHandler(GetAboutDialogTextEventArgs evArgs);
		public event GetAboutDialogTextHandler GetAboutDialogText;
        public delegate void ShowSpriteManagerHandler(int spriteNumber, ref bool successful);
        public event ShowSpriteManagerHandler ShowSpriteManager;
        public delegate void FileChangedInGameFolderHandler(string fileName);
        public event FileChangedInGameFolderHandler FileChangedInGameFolder;
        public event ParameterlessDelegate BuildAllPlatforms;

		public void OnGameSettingsChanged()
		{
			if (GameSettingsChanged != null)
			{
				GameSettingsChanged();
			}
		}

		public void OnImportedOldGame()
		{
			if (ImportedOldGame != null)
			{
				ImportedOldGame();
			}
		}

		public void OnGameLoad(XmlNode rootNode)
		{
			if (GameLoad != null)
			{
				GameLoad(rootNode);
			}
		}

		public void OnSavingGame(XmlTextWriter writer)
		{
			if (SavingGame != null)
			{
				SavingGame(writer);
			}
		}

        public void OnSavingUserData(XmlTextWriter writer)
        {
            if (SavingUserData != null)
            {
                SavingUserData(writer);
            }
        }

        public void OnLoadedUserData(XmlNode rootNode)
        {
            if (LoadedUserData != null)
            {
                LoadedUserData(rootNode);
            }
        }
        
        public void OnRefreshAllComponentsFromGame()
		{
			if (RefreshAllComponentsFromGame != null)
			{
				RefreshAllComponentsFromGame();
			}
		}

		public void OnGetAboutDialogText(GetAboutDialogTextEventArgs evArgs)
		{
			if (GetAboutDialogText != null)
			{
				GetAboutDialogText(evArgs);
			}
		}

        public bool OnShowSpriteManager(int spriteNumber)
        {
            bool successful = false;
            if (ShowSpriteManager != null)
            {
                ShowSpriteManager(spriteNumber, ref successful);
            }
            return successful;
        }

        public void OnFileChangedInGameFolder(string fileName)
        {
            if (FileChangedInGameFolder != null)
            {
                FileChangedInGameFolder(fileName);
            }
        }

        public void OnBuildAllPlatforms()
        {
            if (BuildAllPlatforms != null)
            {
                BuildAllPlatforms();
            }
        }

		private static EditorEvents _instance;

		public static EditorEvents Instance
		{
			get
			{
				if (_instance == null)
				{
					_instance = new EditorEvents();
				}
				return _instance;
			}
		}
	}
}
