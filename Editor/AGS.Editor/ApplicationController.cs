using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Editor.Components;
using AGS.Types;

namespace AGS.Editor
{
    internal class ApplicationController
    {
        private AGSEditor _agsEditor;
        private GUIController _guiController;
        private ComponentController _componentController;
        private NativeProxy _nativeProxy;
        private EditorEvents _events;
        private AGSEditorController _pluginEditorController;
        private string[] _commandLineArgs;
        CommandLineOptions _commandOptions = new CommandLineOptions();

        public ApplicationController(string[] commandLineArguments)
        {
            // Parse and initialize command line options.
            // This must be done prior to any other components, as we must init
            // few things related to the message handling.
            InitializeCommandLineOptions(commandLineArguments);

            _events = Factory.Events;
            _agsEditor = Factory.AGSEditor;
            _guiController = Factory.GUIController;
            _componentController = Factory.ComponentController;
            _nativeProxy = Factory.NativeProxy;
            _pluginEditorController = new AGSEditorController(_componentController, _agsEditor, _guiController);

            _events.GameLoad += _events_GameLoad;
            _events.GamePostLoad += _events_GamePostLoad;
            _events.GameSettingsChanged += _events_GameSettingsChanged;
            _events.ImportedOldGame += _events_ImportedOldGame;
            _events.RefreshAllComponentsFromGame += _events_RefreshAllComponentsFromGame;
            _events.SavingGame += _events_SavingGame;
            _events.SavingUserData += _events_SavingUserData;
            _events.LoadedUserData += _events_LoadedUserData;
            _agsEditor.PreSaveGame += _agsEditor_PreSaveGame;

            _guiController.OnEditorShutdown += GUIController_OnEditorShutdown;
            _guiController.Initialize(_agsEditor);
            _agsEditor.DoEditorInitialization();
            CreateComponents();
        }

        private void InitializeCommandLineOptions(string[] commandLineArguments)
        {
            _commandLineArgs = commandLineArguments;
            _commandOptions = new CommandLineOptions(_commandLineArgs);

            // In case of autonomous operation request: switch to console logging mode;
            // in this mode GUIController will not display modal message dialogs.
            if (_commandOptions.AutoOperationRequested)
            {
                StdConsoleWriter.Enable();
            }

            // Report any invalid command options
            if (_commandOptions.UnknownArgs.Count > 0)
            {
                StringBuilder sb = new StringBuilder();
                foreach (var opt in _commandOptions.UnknownArgs)
                {
                    sb.Append($"    {opt}");
                    sb.Append(Environment.NewLine);
                }

                ShowMessage($"Invalid command line arguments:{Environment.NewLine}{sb.ToString()}", "Adventure Game Studio", MessageBoxIcon.Warning);
            }
        }

        private void _events_LoadedUserData(XmlNode rootNode)
        {
            _componentController.NotifyLoadedUserData(rootNode);
        }

        private void _events_SavingUserData(XmlTextWriter writer)
        {
            _componentController.NotifySavingUserData(writer);
        }

        private void _events_SavingGame(XmlTextWriter writer)
        {
            _componentController.NotifySavingGame(writer);
        }

        private void _events_RefreshAllComponentsFromGame()
        {
            _componentController.NotifyDataRefreshNeeded();
        }

        private void _events_ImportedOldGame()
        {
            _componentController.NotifyImportedOldGame();
        }

        private void _events_GameSettingsChanged()
        {
            _nativeProxy.GameSettingsChanged(_agsEditor.CurrentGame);
            _componentController.NotifyGameSettingsChanged();

            DataFileWriter.TextEncoding = _agsEditor.CurrentGame.TextEncoding;
        }

        private void _events_GameLoad(XmlNode rootNode)
        {
            List<string> missingComponents = _componentController.NotifyLoadedGameAndReturnMissingComponents(rootNode);
            foreach (string componentID in missingComponents)
            {
                _guiController.ShowMessage("This game contains data from a plugin or component '" + componentID + "' which you do not have installed. If you save the game, this data will be lost.", AGS.Types.MessageBoxIconType.Warning);
            }

            DataFileWriter.TextEncoding = _agsEditor.CurrentGame.TextEncoding;
        }

        private void _events_GamePostLoad(Game game)
        {
            // TODO: this may be noticably slow especially for sprites. Display some kind of
            // progress window to notify user.
            // Convert absolute paths to relative paths. This is an automatic fixup from when the
            // editor stored absolute paths only
            foreach (Sprite sprite in game.RootSpriteFolder.GetAllSpritesFromAllSubFolders())
            {
                sprite.SourceFile = Utilities.GetRelativeToProjectPath(sprite.SourceFile);
            }
            foreach (Types.FontFile ff in game.FontFiles)
            {
                ff.SourceFilename = Utilities.GetRelativeToProjectPath(ff.SourceFilename);
            }
            foreach (AudioClip audio in game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                audio.SourceFileName = Utilities.GetRelativeToProjectPath(audio.SourceFileName);
                audio.CacheFileName = AudioComponent.GetCacheFileName(audio);
            }
        }

        private void _agsEditor_PreSaveGame(PreSaveGameEventArgs evArgs)
        {
            _componentController.NotifyAboutToSaveGame();
        }

        private void CreateComponents()
        {
            _componentController.AddComponent(new FileCommandsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new BuildCommandsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new HelpCommandsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new SettingsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new DefaultSetupComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new PaletteComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new SpriteManagerComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new TextParserComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new LipSyncComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new GuiComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new InventoryComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new DialogsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new ViewsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new CharactersComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new CursorsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new FontsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new AudioComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new GlobalVariablesComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new ScriptsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new PluginsComponent(_guiController, _agsEditor, _pluginEditorController));
            _componentController.AddComponent(new RoomsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new TranslationsComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new SpeechComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new WelcomeComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new DebugLogComponent(_guiController, _agsEditor));
            //
            // Disabled until proper server is found to store these stats
            // _componentController.AddComponent(new StatisticsSenderComponent(_guiController, _agsEditor));
            //
            // This is a hack really, to prevent warnings when loading old games;
            // but it's not ideal, as in theory a plugin may introduce similar name;
            // find a better solution later?
            _componentController.AddSuppressedComponent(ComponentIDs.SourceControl);
        }

        /// <summary>
        /// Displays a message either using a message box, or prints one to the console,
        /// if one is enabled.
        /// </summary>
        public static void ShowMessage(string message, string title, MessageBoxIcon icon)
        {
            if (StdConsoleWriter.IsEnabled)
            {
                StdConsoleWriter.WriteLine(message);
                return;
            }

            MessageBox.Show(message, title, MessageBoxButtons.OK, icon);
        }

        public void StartGUI()
        {
            _agsEditor.ApplicationStarted = true;
            _guiController.StartGUI(_commandOptions);
        }

        private Bitmap CaptureScreenshot()
        {
            try
            {
                Form formToCapture = null;
                if (Application.OpenForms.Count > 0)
                {
                    formToCapture = Application.OpenForms[0];
                }
                if (formToCapture != null)
                {
                    Bitmap screenShot = new Bitmap(formToCapture.Width, formToCapture.Height);
                    Graphics g = Graphics.FromImage(screenShot);
                    g.CopyFromScreen(formToCapture.DesktopLocation, new Point(0, 0), screenShot.Size);
                    g.Dispose();
                    return screenShot;
                }
            }
            catch (Exception)
            {
                // this error isn't important, we want to concentrate
                // on the main exception instead
            }
            return null;
        }

        public void UnhandledErrorHandler(Exception ex)
        {
            try
            {
                Bitmap screenShot = null; // disable CaptureScreenshot();
                ExceptionDialog dialog = new ExceptionDialog(ex, screenShot);
                dialog.ShowDialog();
                dialog.Dispose();
                // screenShot.Dispose();
            }
            catch
            {
                _guiController.ShowMessage("An unexpected error occurred. Please post the following information on the AGS Technical Forum." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Stop);
            }
            _guiController.ShowMessage("A serious error occurred and the AGS Editor may now be in an unstable state. You are STRONGLY ADVISED to shut down the editor and restart it. Before saving your work, make a backup copy of your game folder in case any data has been corrupted."
                 + Environment.NewLine + Environment.NewLine + "Error: " + ex.ToString(), MessageBoxIcon.Error);
        }

        private void GUIController_OnEditorShutdown()
        {
            _componentController.ShutdownComponents();

            foreach (IDisposable component in _componentController.Components.OfType<IDisposable>())
            {
                component.Dispose();
            }

            _nativeProxy.Dispose();
        }

    }
}
