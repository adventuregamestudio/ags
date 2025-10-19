using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AGS.Types;

namespace AGS.Editor.Components
{
    class CustomPropertiesComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "CustomProperties";
        private const string ICON_KEY = "CustomPropertiesIcon";
        private const string COMMAND_SHOW_CUSTOM_PROPERTY_SCHEMA_EDITOR = "ShowCustomPropertySchemaEditor";

        public CustomPropertiesComponent(GUIController guiController, AGSEditor agsEditor)
           : base(guiController, agsEditor)
        {
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("customproperties.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Custom Properties", ICON_KEY);
        }

        private void ShowSchemaEditor()
        {
            CustomPropertySchemaEditor schemaEditor = new CustomPropertySchemaEditor(_agsEditor.CurrentGame.PropertySchema);
            schemaEditor.ShowDialog();
            schemaEditor.Dispose();
        }

        public override void CommandClick(string controlID)
        {
            // for now there is only one command, so either double clicking
            // or the context menu will do the same, so we don't need an if check
            ShowSchemaEditor();
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_SHOW_CUSTOM_PROPERTY_SCHEMA_EDITOR, "Edit Schema...", null));
            }
            return menu;
        }

        public override string ComponentID
        {
            get { return ComponentIDs.CustomProperties; }
        }
    }
}
