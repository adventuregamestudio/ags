using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameVoiceClips renames old-style voice clips to new-style format.
    /// </summary>
    public class UpgradeGameVoiceClipsTask : UpgradeGameTaskBase
    {
        internal delegate void ProcessSpeechFiles(Game game, IWorkProgress progress, CompileMessages errors);
        private ProcessSpeechFiles _procSpeechFiles;

        internal UpgradeGameVoiceClipsTask(ProcessSpeechFiles proc) : base("UpgradeGameVoiceClips")
        {
            Title = "Convert old-style voice clips";
            GameVersion = new System.Version("4.0.0.33");
            GameVersionIndex = 4000033;
            Implicit = false;
            Optional = true;
            AllowToSkipIfHadErrors = true;
            RequestConfirmationOnErrors = true;
            Enabled = true;

            _procSpeechFiles = proc;
        }

        /// <summary>
        /// Tells whether this task should be applied to this game.
        /// This method can have additional conditions, besides the default version check.
        /// </summary>
        public override bool ShouldApplyToGame(Game game)
        {
#pragma warning disable 0612
            return game.Settings.UseOldVoiceClipNaming;
#pragma warning restore 0612
        }

        /// <summary>
        /// Provides WizardPage controls used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public override UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpgradeGameVoiceClipsPage(game, this) };
        }
        /// <summary>
        /// Execute the upgrade step over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public override void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            _procSpeechFiles(game, progress, errors);
        }
    }
}
