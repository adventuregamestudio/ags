using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class RecentlyEditedGame
    {
        public string DirectoryPath;
        public string GameName;

        public RecentlyEditedGame(string path, string gameName)
        {
            DirectoryPath = path;
            GameName = gameName;
        }
    }
}
