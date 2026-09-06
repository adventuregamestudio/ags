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
    public class UpgradeGameRoomsOptionalTask : UpgradeGameTaskBase
    {
        internal delegate void ProcessRooms(Game game, RoomsComponent.UpgradeOptions options, IWorkProgress progress, CompileMessages errors);
        private ProcessRooms _processRooms;
        private RoomsComponent.UpgradeOptions _options = new RoomsComponent.UpgradeOptions();

        internal UpgradeGameRoomsOptionalTask(ProcessRooms processRooms) : base("UpgradeGameRoomsOptional")
        {
            Title = "Update Rooms (optional section)";
            GameVersion = new System.Version(AGSEditor.FIRST_USER_DATA_VERSION_WITHOUT_INDEX);
            Implicit = false;
            Optional = true;
            AllowToSkipIfHadErrors = true;
            RequestConfirmationOnErrors = true;
            Enabled = true;

            _processRooms = processRooms;
        }

        // TODO: can we alternatively (optionally) use IUpgradeGameTask.ApplyOptions ?
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
        public override UpgradeGameWizardPage[] CreateWizardPages(Game game)
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
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public override void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            // Check if any option is enabled, prevents from unnecessary loading every room
            if (!_options.AdjustObjectsBy1YPixel)
                return;

            _processRooms(game, _options, progress, errors);
        }
    }
}
