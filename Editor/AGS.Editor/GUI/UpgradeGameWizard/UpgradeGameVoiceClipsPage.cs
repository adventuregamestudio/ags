using AGS.Editor.Components;
using AGS.Types;
using System;

namespace AGS.Editor
{
    public partial class UpgradeGameVoiceClipsPage : UpgradeGameWizardPage
    {
        private UpgradeGameVoiceClipsTask _task;

        public UpgradeGameVoiceClipsPage(Game game, UpgradeGameVoiceClipsTask task)
            : base(game, task)
        {
            InitializeComponent();
            _task = task;
            chkRenameVoiceClips.Checked = _task.Enabled;
        }

        public override bool NextButtonPressed()
        {
            _task.Enabled = chkRenameVoiceClips.Checked;
            return true;
        }
    }
}
