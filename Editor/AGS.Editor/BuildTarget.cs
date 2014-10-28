using AGS.Types;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTarget
    {
        /// <summary>
        /// Returns the target platforms selected for building, or null if no game is loaded yet.
        /// </summary>
        public static BuildTargetPlatform? TargetedPlatforms
        {
            get
            {
                return (AGSEditor.Instance.CurrentGame == null ? (BuildTargetPlatform?)null : AGSEditor.Instance.CurrentGame.Settings.BuildTargets);
            }
        }
    }
}
