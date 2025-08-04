using System;
using System.Collections.Generic;
using AGS.Types;
using AGS.Editor.Components;

namespace AGS.Editor
{
    /// <summary>
    /// Performs an optional all-rooms update, according to the user
    /// selection (made in related game wizard pages).
    /// </summary>
    public class UpgradeGameRoomsOptionalTask : IUpgradeGameTask
    {
        internal delegate void ProcessRooms(Game game, RoomsComponent.UpgradeOptions options, IWorkProgress progress, CompileMessages errors);
        private ProcessRooms _processRooms;
        private RoomsComponent.UpgradeOptions _options = new RoomsComponent.UpgradeOptions();

        internal UpgradeGameRoomsOptionalTask(ProcessRooms processRooms)
        {
            _processRooms = processRooms;
            Enabled = true;
        }

        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        public string ID { get { return "UpgradeGameRoomsOptional"; } }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Update Rooms (optional section)"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description
        {
            get { return ""; }
        }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        public System.Version GameVersion { get { return new System.Version(AGSEditor.FIRST_USER_DATA_VERSION_WITHOUT_INDEX); } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return null; } }
        /// <summary>
        /// Tells whether this upgrade task is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get { return false; } }
        /// <summary>
        /// Tells whether this upgrade task may be disabled by user's choice.
        /// </summary>
        public bool Optional { get { return true; } }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// task had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get { return true; } }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this task had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get { return true; } }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        internal RoomsComponent.UpgradeOptions Options
        {
            get { return _options; }
            set { _options = value; }
        }

        /// <summary>
        /// Provides WizardPage controls used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            // NOTE: we may use game.SavedXmlVersion to decide which pages
            // and/or options to display! Return null if none are necessary.
            List<UpgradeGameWizardPage> pages = new List<UpgradeGameWizardPage>();
            if (game.SavedXmlVersion < (new System.Version(AGSEditor.FIRST_USER_DATA_VERSION_WITHOUT_INDEX)))
            {
                pages.Add(new UpdateGameRoomsOptionalPage(game, this));
            }
            // If no options are required, then auto-disable ourselves
            // CHECKME: may be not the best place to do this, revise this later...
            if (pages.Count == 0)
                Enabled = false;
            return pages.ToArray();
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {
            // does not have any options
        }
        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            // Check if any option is enabled, prevents from unnecessary loading every room
            if (!_options.AdjustObjectsBy1YPixel)
                return;

            _processRooms(game, _options, progress, errors);
        }
    }
}
