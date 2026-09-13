using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    public class UpgradeGameRoomsOpenFormatTask : UpgradeGameTaskBase
    {
        // TODO: revise this later.
        // The conversion process is run via a delegate here, because at the time
        // I was not certain if it should remain in RoomComponent or not.
        // It sort of makes sense, conceptually.
        // Also, the conversion calls a number of private methods from RoomComponent,
        // therefore this seemed to be the most trivial way to proceed.
        public delegate void ConvertRooms(Game game, IWorkProgress progress, CompileMessages errors);
        private ConvertRooms _convertRooms;

        public UpgradeGameRoomsOpenFormatTask(ConvertRooms convertRooms) : base("UpgradeGameRoomsOpenFormat")
        {
            Enabled = true;
            Title = "Upgrade Rooms to the new open format";
            Description = "In AGS 4.0 the Rooms will now be stored in a \"open\" format, where each Room has a subfolder inside the \"Rooms\" folder, and each room component is saved as a separate file: properties are saved as XML, backgrounds and masks as PNG files, and so forth." +
                    Environment.NewLine + Environment.NewLine +
                    "During this upgrade step every one of your \"room*.crm\" files will be converted into this open format representation." +
                    Environment.NewLine + Environment.NewLine +
                    "After this upgrade the \"room*.crm\" files will become purely output files, created as a result of the game compilation. They no longer need to be kept in the project folder. You may even delete them when e.g. sending your game sources to a co-developer in order to save disk space, and ignore them when adding your project under a source control.";
            GameVersion = new System.Version(AGSEditor.FIRST_XML_VERSION_USING_INDEX);
            GameVersionIndex = AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_OPEN_ROOMS;
            Implicit = false;
            Optional = false;
            AllowToSkipIfHadErrors = false;
            RequestConfirmationOnErrors = false;


            _convertRooms = convertRooms;
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
            _convertRooms(game, progress, errors);
        }
    }
}
