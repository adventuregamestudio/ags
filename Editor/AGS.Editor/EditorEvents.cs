using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Editor
{
    internal class EditorEvents
    {
        public event Action GameSettingsChanged;
        public event Action ImportedOldGame;
        public event Action RefreshAllComponentsFromGame;
        public delegate void GameLoadHandler(XmlNode rootNode);
        public event GameLoadHandler GameLoad;
        public delegate void GamePrepareUpgradeHandler(UpgradeGameEventArgs args);
        public event GamePrepareUpgradeHandler GamePrepareUpgrade;
        public delegate void GamePostLoadHandler(Game game);
        public event GamePostLoadHandler GamePostLoad;
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
        public delegate void SpriteImportHandler(int[] spriteNumbers);
        public event SpriteImportHandler SpritesImported;

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

        public void OnGamePrepareUpgrade(UpgradeGameEventArgs args)
        {
            GamePrepareUpgrade?.Invoke(args);
        }

        public void OnGamePostLoad(Game game)
        {
            if (GamePostLoad != null)
            {
                GamePostLoad(game);
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

        /// <summary>
        /// Notifies components about sprites being (re)imported.
        /// spriteNumbers either contains a list of updated sprite IDs,
        /// but is allowed to be null, in which case we should assume that
        /// there was a large bulk of changes to the sprite assets.
        /// </summary>
        public void OnSpritesImported(int[] spriteNumbers)
        {
            SpritesImported?.Invoke(spriteNumbers);
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
