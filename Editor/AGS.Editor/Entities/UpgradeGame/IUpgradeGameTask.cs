using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// IUpgradeGameTask represents a single step operation in the game upgrade
    /// process. The operation itself may include multiple adjustments to the
    /// game's data. A distinct upgrade task is required when particular changes
    /// are optional, or may require additional setup by user. It also makes
    /// sense to have a task if you like to simply warn user about some changes,
    /// in which case you may want to provide a Upgrade Wizard page with a
    /// text, describing which changes will be done to a project.
    ///
    /// Upgrade Task is defined by a title, may have a description, and a
    /// WizardPage which lets user to read and configure this task.
    /// Upgrade Task may be marked as "Implicit", in which case it's never
    /// shown to user and is performed unconditionally.
    /// Upgrade Task may be marked as "Optional", in which case user is
    /// let to disable its execution.
    /// </summary>
    public interface IUpgradeGameTask
    {
        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        string ID { get; }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        string Title { get; }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        string Description { get; }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        System.Version GameVersion { get; }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        int? GameVersionIndex { get; }
        /// <summary>
        /// Tells whether this upgrade task is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        bool Implicit { get; }
        /// <summary>
        /// Tells whether this upgrade task may be disabled by user's choice.
        /// </summary>
        bool Optional { get; }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// task had errors.
        /// </summary>
        bool AllowToSkipIfHadErrors { get; }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this task had errors.
        /// </summary>
        bool RequestConfirmationOnErrors { get; }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        bool Enabled { get; set; }

        /// <summary>
        /// Provides a WizardPage control used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        UpgradeGameWizardPage CreateWizardPage(Game game);
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        void ApplyOptions(Dictionary<string, string> options);
        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        void Execute(Game game, IWorkProgress progress, CompileMessages errors);
    }
}
