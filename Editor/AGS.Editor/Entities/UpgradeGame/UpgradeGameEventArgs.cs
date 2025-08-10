using System;
using System.Collections.Generic;

namespace AGS.Editor
{
    /// <summary>
    /// Lets editor components provide additional upgrade tasks
    /// </summary>
    public class UpgradeGameEventArgs
    {
        public List<IUpgradeGameTask> Tasks { get; private set; }

        public UpgradeGameEventArgs(List<IUpgradeGameTask> tasks)
        {
            Tasks = tasks;
        }
    }
}
