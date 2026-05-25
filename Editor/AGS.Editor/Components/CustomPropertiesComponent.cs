using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor.Components
{
    class CustomPropertiesComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "CustomProperties";
        private const string ICON_KEY = "CustomPropertiesIcon";
        private const string COMMAND_SHOW_CUSTOM_PROPERTY_SCHEMA_EDITOR = "ShowCustomPropertySchemaEditor";
        private const string COMMAND_IMPORT_CUSTOM_PROPERTIES = "ImportCustomProperties";
        private const string COMMAND_EXPORT_CUSTOM_PROPERTIES = "ExportCustomProperties";

        private const string SCHEMA_EXPORT_FILE_FILTER = "AGS Custom Properties Schema (*.cpschema)|*.cpschema";
        private const string SCHEMA_IMPORT_FILE_FILTER = "AGS Exported Custom Properties Schema (*.cpschema)|*.cpschema";

        public const string SCHEMA_FILE_ROOT_NODE = "CustomPropertiesSchema";

        public CustomPropertiesComponent(GUIController guiController, AGSEditor agsEditor)
           : base(guiController, agsEditor)
        {
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("customproperties.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Custom Properties", ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.CustomProperties; }
        }

        public override void BeforeSaveGame()
        {
            SyncCustomPropertiesWithSchema();
        }

        public override void RefreshDataFromGame()
        {
            SyncCustomPropertiesWithSchema();
        }

        private void SyncCustomPropertiesWithSchema()
        {
            // Note that this syncs only global objects.
            // Room elements cannot be synced without loading each and every room into memory,
            // which could be a pretty long process. So they are dealt with separately, by the RoomsComponent.
            foreach (var c in _agsEditor.CurrentGame.Characters)
                c.Properties.SyncWithSchema();
            foreach (var i in _agsEditor.CurrentGame.InventoryItems)
                i.Properties.SyncWithSchema();
        }

        private void ShowSchemaEditor()
        {
            List<CustomPropertyDefChange> schemaChanges = null;
            var schemaCopy = new CustomPropertySchema(_agsEditor.CurrentGame.PropertySchema);
            CustomPropertySchemaEditor schemaEditor = new CustomPropertySchemaEditor(schemaCopy);
            if (schemaEditor.ShowDialog() == DialogResult.OK)
            {
                schemaChanges = schemaEditor.SchemaChanges;
            }
            schemaEditor.Dispose();

            if (schemaChanges != null)
            {
                // Filter out name changes - these are the only changes we want to react to.
                var wantedChanges = schemaChanges.Where(
                    sc => (sc.Type == CustomPropertyDefChangeType.Edit && sc.OriginalName != sc.NewName)).ToArray();
                if (wantedChanges.Length == 0)
                {
                    _agsEditor.CurrentGame.PropertySchema.CopyFrom(schemaCopy);
                    return;
                }

                if (GUIController.Instance.ShowQuestion($"You have renamed some of the custom properties. Editor will have to apply modifications to each object in game which has these properties, including objects in Rooms. This process may take some time, because each Room will have to be loaded and resaved.{Environment.NewLine}{Environment.NewLine}"
                                                     + "If you cancel this operation now, the Custom Properties Schema will be reverted to its previous state in order to prevent mismatches.",
                                                     MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    _agsEditor.CurrentGame.PropertySchema.CopyFrom(schemaCopy);
                    _agsEditor.Tasks.HandleCustomPropertySchemaChanges(schemaChanges);
                    Factory.GUIController.RefreshPropertyGrid();
                }
            }
        }

        private void ExportSchema()
        {
            string fileName = _guiController.ShowSaveFileDialog("Export custom properties schema as...", SCHEMA_EXPORT_FILE_FILTER);
            if (fileName == null)
                return;
            
            CustomPropertySchema schema = _agsEditor.CurrentGame.PropertySchema;
            Game game = _agsEditor.CurrentGame;
            try
            {
                ImportExport.ExportCustomPropertiesSchemaToFile(schema, fileName, game);
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowError("An error occurred exporting the custom properties schema file.", ex, MessageBoxIcon.Warning);
            }
        }

        private void ImportSchema()
        {
            string fileName = _guiController.ShowOpenFileDialog("Select custom properties schema file to import...", SCHEMA_IMPORT_FILE_FILTER);
            if (fileName == null)
                return;

            Game game = _agsEditor.CurrentGame;
            try
            {
                var schema = ImportExport.ImportCustomPropertiesSchemaFromFile(fileName, game);
                // TODO: implement a dialog that lets user to select which properties to merge and which to skip

                int addCount, skipCount;
                MergeToGameSchema(schema, out addCount, out skipCount);
                // refresh property grid, a property may have been added, changed or removed
                Factory.GUIController.RefreshPropertyGrid();
                string result = $"Custom properties schema imported successfully:\n - {addCount} properties added"
                    + ((skipCount > 0) ?
                        $";\n - {skipCount} properties skipped because their names match existing ones." :
                        ".");
                _guiController.ShowMessage(result, MessageBoxIcon.Information);
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowError("An error occurred importing the custom properties schema file.", ex, MessageBoxIcon.Warning);
            }
        }

        private void MergeToGameSchema(CustomPropertySchema schema, out int addCount, out int skipCount)
        {
            addCount = 0;
            skipCount = 0;
            foreach (var propDef in schema.PropertyDefinitions)
            {
                var match = _agsEditor.CurrentGame.PropertySchema.PropertyDefinitions.Find(x => x.Name == propDef.Name);
                if (match == null)
                {
                    _agsEditor.CurrentGame.PropertySchema.PropertyDefinitions.Add(propDef);
                    addCount++;
                }
                else
                {
                    skipCount++;
                }
            }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID || controlID == COMMAND_SHOW_CUSTOM_PROPERTY_SCHEMA_EDITOR)
            {
                // should show schema editor on either double click or selecting edit schema in right click menu
                ShowSchemaEditor();
            }
            else if (controlID == COMMAND_IMPORT_CUSTOM_PROPERTIES)
            {
                ImportSchema();
            }
            else if (controlID == COMMAND_EXPORT_CUSTOM_PROPERTIES)
            {
                ExportSchema();
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_SHOW_CUSTOM_PROPERTY_SCHEMA_EDITOR, "Edit Schema...", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_IMPORT_CUSTOM_PROPERTIES, "Import schema...", null));
                menu.Add(new MenuCommand(COMMAND_EXPORT_CUSTOM_PROPERTIES, "Export schema...", null));
            }
            return menu;
        }
    }
}
