using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Editor.Components
{
    public class PluginsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Plugins";
        private const string MENU_ITEM_ENABLED = "EnablePlugin";
        private const string MENU_ITEM_PROPERTIES = "PluginProperties";
        private const string PLUGIN_SEARCH_MASK = "ags*.dll";

        private List<NativePlugin> _plugins = new List<NativePlugin>();
		private List<EditorPlugin> _editorPlugins = new List<EditorPlugin>();
        private NativePlugin _rightClickedPlugin;
		private AGSEditorController _pluginEditorController;

        public PluginsComponent(GUIController guiController, AGSEditor agsEditor, AGSEditorController pluginEditorController)
            : base(guiController, agsEditor)
        {
			_pluginEditorController = pluginEditorController;

            foreach (string fullFileName in Utilities.GetDirectoryFileList(_agsEditor.EditorDirectory, PLUGIN_SEARCH_MASK))
            {
                string fileName = Path.GetFileName(fullFileName).ToLower();
                // Don't process our editor .net dlls
                if (!fileName.StartsWith("ags."))
                {
                    LoadRuntimePluginIntoMemory(fileName);
                }
				else if (fileName.StartsWith("ags.plugin."))
				{
					LoadEditorPluginIntoMemory(fileName);
				}
            }

            _guiController.RegisterIcon("PluginIcon", Resources.ResourceManager.GetIcon("plugin.ico"));
            _guiController.RegisterIcon("PluginDisabledIcon", Resources.ResourceManager.GetIcon("pluginDisabled.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Plugins", "PluginIcon");
            RepopulateTreeView();

            _agsEditor.GetScriptHeaderList += new GetScriptHeaderListHandler(AGSEditor_GetScriptHeaderList);
            _agsEditor.Tasks.GetFilesForInclusionInTemplate += new Tasks.GetFilesForInclusionInTemplateHandler(AGSEditor_GetFilesForInclusionInTemplate);
            _agsEditor.Tasks.NewGameFilesExtracted += new Tasks.NewGameFilesExtractedHandler(AGSEditor_NewGameFilesExtracted);
			Factory.Events.GetAboutDialogText += new EditorEvents.GetAboutDialogTextHandler(Events_GetAboutDialogText);
        }

		private void Events_GetAboutDialogText(GetAboutDialogTextEventArgs evArgs)
		{
			if (_editorPlugins.Count > 0)
			{
				evArgs.Text += "Editor plugins loaded:" + Environment.NewLine;
				foreach (EditorPlugin plugin in _editorPlugins)
				{
					evArgs.Text += "* " + plugin.FileName + Environment.NewLine;
				}
			}
		}

        private void LoadRuntimePluginIntoMemory(string fileName)
        {
            try
            {
                NativePlugin plugin = new NativePlugin(fileName);
                _plugins.Add(plugin);
            }
            catch (AGSEditorException ex)
            {
                _guiController.ShowMessage("There was an error loading plugin '" + fileName + "'." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("There was an error loading plugin '" + fileName + "'." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Warning);
            }
        }

		private void LoadEditorPluginIntoMemory(string fileName)
		{
			try
			{
				EditorPlugin plugin = new EditorPlugin(fileName, _pluginEditorController);
				_editorPlugins.Add(plugin);
			}
			catch (AGSEditorException ex)
			{
				_guiController.ShowMessage("There was an error loading plugin '" + fileName + "'." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
			}
			catch (Exception ex)
			{
				_guiController.ShowMessage("There was an error loading plugin '" + fileName + "'." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Warning);
			}
		}

        public override void EditorShutdown()
        {
			foreach (EditorPlugin plugin in _editorPlugins)
			{
				plugin.Dispose();
			}

			foreach (NativePlugin plugin in _plugins)
            {
                plugin.Dispose();
            }

			_editorPlugins.Clear();
			_plugins.Clear();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Plugins; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                // double-click on main Plugins node, do nothing
            }
            else if (controlID == MENU_ITEM_ENABLED)
            {
                _rightClickedPlugin.Enabled = !_rightClickedPlugin.Enabled;
                RepopulateTreeView();
                _guiController.ProjectTree.SelectNode(this, "Plg" + _rightClickedPlugin.FileName);
            }
            else if (controlID == MENU_ITEM_PROPERTIES)
            {
                _rightClickedPlugin.ShowPropertiesWindow(_guiController.TopLevelWindowHandle);
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                return null;
            }
            IList<MenuCommand> contextMenu = new List<MenuCommand>();
            _rightClickedPlugin = GetPluginByFileName(controlID.Substring(3));

            MenuCommand useThisPluginCmd = new MenuCommand(MENU_ITEM_ENABLED, "Use this plugin", null);
            useThisPluginCmd.Checked = _rightClickedPlugin.Enabled;
            contextMenu.Add(useThisPluginCmd);

            MenuCommand propertiesCmd = new MenuCommand(MENU_ITEM_PROPERTIES, "Plugin properties...", null);
            propertiesCmd.Enabled = (_rightClickedPlugin.HasProperties && _rightClickedPlugin.Enabled);
            contextMenu.Add(propertiesCmd);

            return contextMenu;
        }

        public override void BeforeSaveGame()
        {
            List<Plugin> gamePlugins = _agsEditor.CurrentGame.Plugins;
            gamePlugins.Clear();
            foreach (NativePlugin plugin in _plugins)
            {
                if (plugin.Enabled)
                {
                    gamePlugins.Add(new Plugin(plugin.FileName, plugin.SerializeData()));
                }
            }
        }

        public override void RefreshDataFromGame()
        {
            // Disable all plugins before loading the game
            foreach (NativePlugin nativePlugin in _plugins) 
            {
                nativePlugin.Enabled = false;
            }

            foreach (Plugin gamePlugin in _agsEditor.CurrentGame.Plugins)
            {
                NativePlugin nativePlugin = GetPluginByFileName(gamePlugin.FileName.ToLower());
                if (nativePlugin == null)
                {
                    _guiController.ShowMessage("This game uses the plugin '" + gamePlugin.FileName + "', which was not found. If you save the game now, all references to this plugin will be removed.", MessageBoxIcon.Warning);
                }
                else
                {
                    nativePlugin.Enabled = true;
                    nativePlugin.DeserializeData(gamePlugin.SerializedData);
                }
            }
            RepopulateTreeView();
        }

        private void RepopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (NativePlugin plugin in _plugins)
            {
                string iconToUse = "PluginDisabledIcon";
                if (plugin.Enabled)
                {
                    iconToUse = "PluginIcon";
                }
                _guiController.ProjectTree.AddTreeLeaf(this, "Plg" + plugin.FileName, plugin.PluginName + " (" + plugin.FileName + ")", iconToUse, !plugin.Enabled);
            }
        }

        private NativePlugin GetPluginByFileName(string fileName)
        {
            foreach (NativePlugin plugin in _plugins)
            {
                if (plugin.FileName == fileName)
                {
                    return plugin;
                }
            }
            return null;
        }

        private void AGSEditor_GetScriptHeaderList(GetScriptHeaderListEventArgs evArgs)
        {
            foreach (NativePlugin plugin in _plugins)
            {
				foreach (Script header in plugin.ScriptHeaders)
				{
					evArgs.ScriptHeaders.Add(header);
				}
            }
        }

        private void AGSEditor_GetFilesForInclusionInTemplate(List<string> fileNames)
        {
            foreach (NativePlugin plugin in _plugins)
            {
                if (plugin.Enabled)
                {
                    fileNames.Add(Path.Combine(_agsEditor.EditorDirectory, plugin.FileName));
                }
            }
        }

        private void AGSEditor_NewGameFilesExtracted()
        {
            string gameDirectory = Directory.GetCurrentDirectory();

            foreach (string filePathName in Utilities.GetDirectoryFileList(gameDirectory, PLUGIN_SEARCH_MASK))
            {
                string fileName = Path.GetFileName(filePathName);
                string destinationPath = Path.Combine(_agsEditor.EditorDirectory, fileName);
                if (File.Exists(destinationPath))
                {
                    _guiController.ShowMessage("This template is trying to install a plugin (" + fileName + "), but you already have a version of this plugin installed.\n\nYour existing version of the plugin will be preserved. You may wish to check to ensure compatibility between versions.", MessageBoxIcon.Warning);
                }
                else
                {
                    File.Move(filePathName, destinationPath);

                    Directory.SetCurrentDirectory(_agsEditor.EditorDirectory);
					LoadRuntimePluginIntoMemory(fileName);
                    Directory.SetCurrentDirectory(gameDirectory);
                }
            }

        }
    }
}
