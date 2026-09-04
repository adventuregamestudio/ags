using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameVoiceClips renames old-style voice clips to new-style format.
    /// </summary>
    public class UpgradeGameVoiceClipsTask : IUpgradeGameTask
    {
        internal delegate void ProcessSpeechFiles(Game game, IWorkProgress progress, CompileMessages errors);
        private ProcessSpeechFiles _procSpeechFiles;

        internal UpgradeGameVoiceClipsTask(ProcessSpeechFiles proc)
        {
            _procSpeechFiles = proc;
            Enabled = true;
        }

        /// <summary>
        /// A unique string identifier of this upgrade step.
        /// </summary>
        public string ID { get { return "UpgradeGameVoiceClips"; } }
        /// <summary>
        /// An arbitrary title, used to identify this step when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Convert old-style voice clips"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description { get { return ""; /* TODO? */ } }
        /// <summary>
        /// A game project version that introduced this upgrade step.
        /// If a loaded game has a less project version, then this step
        /// must be applied, otherwise it should not.
        /// </summary>
        public System.Version GameVersion { get { return new System.Version("4.0.0.33"); } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return 4000033; } }
        /// <summary>
        /// Tells whether this upgrade step is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get { return false; } }
        /// <summary>
        /// Tells whether this upgrade step may be disabled by user's choice.
        /// </summary>
        public bool Optional { get { return true; } }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// step had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get { return true; } }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this step had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get { return true; } }
        /// <summary>
        /// Tells which stage should this task be run on.
        /// </summary>
        public UpgradeGameTaskStage Stage { get { return UpgradeGameTaskStage.None; } }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// Tells whether this task should be applied to this game.
        /// This method can have additional conditions, besides the default version check.
        /// </summary>
        public bool ShouldApplyToGame(Game game)
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
        public UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpgradeGameVoiceClipsPage(game, this) };
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {

        }
        /// <summary>
        /// Execute the upgrade step over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            _procSpeechFiles(game, progress, errors);
        }
    }
}
