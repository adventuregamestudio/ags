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
    public class UpgradeGameRoomsValueRangesTask : UpgradeGameTaskBase
    {
        internal delegate void ProcessRooms(Game game, IWorkProgress progress, CompileMessages errors);
        private ProcessRooms _processRooms;

        internal UpgradeGameRoomsValueRangesTask(ProcessRooms processRooms) : base("UpgradeGameRoomsValueRanges")
        {
            Title = "Update Rooms";
            Description = "Transparency properties (0-100) will be replaced by Opacity (0-255)" + Environment.NewLine + "LightLevel properties (0-200) will be converted to -255-255" + Environment.NewLine + "Tint Luminance and Saturation (0-100) will be converted to 0-255";
            GameVersion = new System.Version("4.0.0.34"); // version where we changed transparency to opacity, and other values to 0-255
            Implicit = false;
            Optional = false;
            AllowToSkipIfHadErrors = true;
            RequestConfirmationOnErrors = true;
            Enabled = true;

            _processRooms = processRooms;
        }

        /// <summary>
        /// Provides WizardPage controls used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public override UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpdateGameGenericInfoPage(game, this) };
        }

        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public override void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            _processRooms(game, progress, errors);
        }
    }
}
