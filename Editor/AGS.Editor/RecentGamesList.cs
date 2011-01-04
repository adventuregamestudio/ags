using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Microsoft.Win32;

namespace AGS.Editor
{
    public class RecentGamesList
    {
        private List<RecentlyEditedGame> _recentGames = new List<RecentlyEditedGame>();

        public RecentGamesList()
        {
            RegistryKey key = Registry.CurrentUser.OpenSubKey(AGSEditor.AGS_REGISTRY_KEY);
            if (key != null)
            {
				string usePrefix = "RecentPath";
				if (key.GetValue("RecentPath0", string.Empty).ToString() == string.Empty)
				{
					// if no 2.8 recent games, read the 2.72 list instead
					usePrefix = "Recent";
				}
                int i = 0;
                string recentPath;
                string recentGameName;
                do
                {
                    recentPath = key.GetValue(usePrefix + i, string.Empty).ToString();
                    recentGameName = key.GetValue("RecentName" + i, string.Empty).ToString();
                    if ((recentPath.Length > 0) && (recentGameName.Length == 0))
                    {
                        recentGameName = Path.GetFileName(recentPath);
                    }
                    if ((recentPath.Length > 0) && (Directory.Exists(recentPath)))
                    {
                        _recentGames.Add(new RecentlyEditedGame(recentPath, recentGameName));
                    }
                    i++;
                }
                while (recentPath.Length > 0);
                key.Close();
            }
        }

        public List<RecentlyEditedGame> RecentGames
        {
            get { return _recentGames; }
        }

        public void AddRecentGame(string directory, string name)
        {
            foreach (RecentlyEditedGame game in _recentGames)
            {
                if (game.DirectoryPath.ToLower() == directory.ToLower())
                {
                    _recentGames.Remove(game);
                    break;
                }
            }

            _recentGames.Insert(0, new RecentlyEditedGame(directory, name));
            SaveRecentGamesList();
        }

        private void SaveRecentGamesList()
        {
            RegistryKey key = Utilities.OpenAGSRegistryKey();
            if (key == null)
            {
                Factory.GUIController.ShowMessage("Unable to access registry key: " + AGSEditor.AGS_REGISTRY_KEY, System.Windows.Forms.MessageBoxIcon.Warning);
            }
            else
            {
                int i = 0;
                foreach (RecentlyEditedGame game in _recentGames)
                {
                    key.SetValue("RecentPath" + i, game.DirectoryPath);
                    key.SetValue("RecentName" + i, game.GameName);
                    i++;
                }
                key.DeleteValue("RecentPath" + i, false);
                key.Close();
            }
        }
    }
}
