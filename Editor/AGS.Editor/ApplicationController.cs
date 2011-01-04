using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Editor.Components;

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

        public ApplicationController()
        {
			_events = Factory.Events;
            _agsEditor = Factory.AGSEditor;
            _guiController = Factory.GUIController;
			_componentController = Factory.ComponentController;
			_nativeProxy = Factory.NativeProxy;
			_pluginEditorController = new AGSEditorController(_componentController, _agsEditor, _guiController);

			_events.GameLoad += new EditorEvents.GameLoadHandler(_events_GameLoad);
			_events.GameSettingsChanged += new EditorEvents.ParameterlessDelegate(_events_GameSettingsChanged);
			_events.ImportedOldGame += new EditorEvents.ParameterlessDelegate(_events_ImportedOldGame);
			_events.RefreshAllComponentsFromGame += new EditorEvents.ParameterlessDelegate(_events_RefreshAllComponentsFromGame);
			_events.SavingGame += new EditorEvents.SavingGameHandler(_events_SavingGame);
            _events.SavingUserData += new EditorEvents.SavingUserDataHandler(_events_SavingUserData);
            _events.LoadedUserData += new EditorEvents.LoadedUserDataHandler(_events_LoadedUserData);
			_agsEditor.PreSaveGame += new AGSEditor.PreSaveGameHandler(_agsEditor_PreSaveGame);

            _guiController.OnEditorShutdown += new GUIController.EditorShutdownHandler(GUIController_OnEditorShutdown);
            _guiController.Initialize(_agsEditor);
            _agsEditor.DoEditorInitialization();
			CreateComponents();
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
		}

		private void _events_GameLoad(XmlNode rootNode)
		{
			List<string> missingComponents = _componentController.NotifyLoadedGameAndReturnMissingComponents(rootNode);
			foreach (string componentID in missingComponents)
			{
				_guiController.ShowMessage("This game contains data from a plugin or component '" + componentID + "' which you do not have installed. If you save the game, this data will be lost.", AGS.Types.MessageBoxIconType.Warning);
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
			_componentController.AddComponent(new SourceControlComponent(_guiController, _agsEditor));
			_componentController.AddComponent(new WelcomeComponent(_guiController, _agsEditor));
            _componentController.AddComponent(new StatisticsSenderComponent(_guiController, _agsEditor));
        }

        public void StartGUI(string[] commandLineArguments)
        {
			_agsEditor.ApplicationStarted = true;
            _guiController.StartGUI(commandLineArguments);
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
                Bitmap screenShot = CaptureScreenshot();
				ExceptionDialog dialog = new ExceptionDialog(ex, screenShot);
				dialog.ShowDialog();
                dialog.Dispose();
                screenShot.Dispose();
			}
			catch
			{
				_guiController.ShowMessage("An unexpected error occurred. Please post the following information on the AGS Technical Forum." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Stop);
			}
            _guiController.ShowMessage("A serious error occurred and the AGS Editor may now be in an unstable state. You are STRONGLY ADVISED to shut down the editor and restart it. Before saving your work, make a backup copy of your game folder in case any data has been corrupted.", MessageBoxIcon.Error);
        }

        private void GUIController_OnEditorShutdown()
        {
            _componentController.ShutdownComponents();
            _nativeProxy.Dispose();
        }

    }
}
