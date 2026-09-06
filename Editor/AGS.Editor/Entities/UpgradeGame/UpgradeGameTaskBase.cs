using AGS.Types;
using System;
using System.Collections.Generic;

namespace AGS.Editor
{
    /// <summary>
    /// An abstract implementation of IUpgradeGameTask.
    /// Lets its descendants assign interface's properties in constructor instead of implementing them.
    /// Descendant class is only oblidged to implement Execute() method, everything else is optional.
    /// </summary>
    public abstract class UpgradeGameTaskBase : IUpgradeGameTask
    {
        private string _id;

        public UpgradeGameTaskBase(string id)
        {
            _id = id;
        }

        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        public string ID { get { return _id; } }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        public string Title { get; protected set; }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description { get; protected set; }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        public System.Version GameVersion { get; protected set; }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get; protected set; }
        /// <summary>
        /// Tells whether this upgrade task is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get; protected set; }
        /// <summary>
        /// Tells whether this upgrade task may be disabled by user's choice.
        /// </summary>
        public bool Optional { get; protected set; }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// task had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get; protected set; }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this task had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get; protected set; }
        /// <summary>
        /// Tells which stage should this task be run on.
        /// </summary>
        public UpgradeGameTaskStage Stage { get; protected set; }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// Tells whether this task should be applied to this game.
        /// This method can have additional conditions, besides the default version check.
        /// </summary>
        public virtual bool ShouldApplyToGame(Game game)
        {
            return true;
        }
        /// <summary>
        /// Provides WizardPage control(s) used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public virtual UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return null;
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public virtual void ApplyOptions(Dictionary<string, string> options)
        {
            // does not have any options
        }
        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public abstract void Execute(Game game, IWorkProgress progress, CompileMessages errors);
    }
}
