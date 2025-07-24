using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// A parent class for the "Upgrade Game Wizard" pages.
    /// Is intended to apply option values to the respective IUpgradeGameTask object.
    /// </summary>
    public class UpgradeGameWizardPage : WizardPage
    {
        private Game _game;
        private IUpgradeGameTask _upgradeTask;
        private bool _taskEnabled = true;

        public Game Game { get { return _game; } }
        public IUpgradeGameTask Task { get { return _upgradeTask; } }
        public bool TaskEnabled { get { return _taskEnabled; } }

        public UpgradeGameWizardPage()
        {
        }

        public UpgradeGameWizardPage(Game game, IUpgradeGameTask task)
        {
            _game = game;
            _upgradeTask = task;
        }
    }
}
