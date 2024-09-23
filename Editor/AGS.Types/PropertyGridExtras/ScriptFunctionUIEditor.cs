using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;
using System.Text;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    public class ScriptFunctionUIEditor : UITypeEditor
    {
        public delegate void OpenScriptEditorHandler(string scriptModule, string functionName);
        public static OpenScriptEditorHandler OpenScriptEditor;
        public delegate void CreateScriptFunctionHandler(string scriptModule, string functionName, string parameters);
        public static CreateScriptFunctionHandler CreateScriptFunction;

        public ScriptFunctionUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            string itemName = string.Empty;
            string scriptModule = Script.GLOBAL_SCRIPT_FILE_NAME;
            if (context.Instance is GUI)
            {
                itemName = ((GUI)context.Instance).Name;
            }
            else if (context.Instance is GUIControl)
            {
                itemName = ((GUIControl)context.Instance).Name;
            }
            else if (context.Instance is InventoryItem)
            {
                itemName = ((InventoryItem)context.Instance).Name;
                scriptModule = ((InventoryItem)context.Instance).Interactions.ScriptModule;
            }
            else if (context.Instance is Character)
            {
                itemName = ((Character)context.Instance).ScriptName;
                scriptModule = ((Character)context.Instance).Interactions.ScriptModule;
            }
            else if (context.Instance is Room)
            {
                itemName = "room";
                scriptModule = ((Room)context.Instance).Interactions.ScriptModule;
            }
            else if (context.Instance is RoomHotspot)
            {
                itemName = ((RoomHotspot)context.Instance).Name;
                scriptModule = ((RoomHotspot)context.Instance).Interactions.ScriptModule;
            }
            else if (context.Instance is RoomObject)
            {
                itemName = ((RoomObject)context.Instance).Name;
                scriptModule = ((RoomObject)context.Instance).Interactions.ScriptModule;
            }
            else if (context.Instance is RoomRegion)
            {
                itemName = "region" + ((RoomRegion)context.Instance).ID;
                scriptModule = ((RoomRegion)context.Instance).Interactions.ScriptModule;
            }
            else
            {
                throw new AGSEditorException($"Invalid object type for editing: {context.Instance.GetType().Name}");
            }

            if (!scriptModule.EndsWith(".asc"))
                scriptModule = scriptModule + ".asc";

            ScriptFunctionParametersAttribute parametersAttribute = ((ScriptFunctionParametersAttribute)context.PropertyDescriptor.Attributes[typeof(ScriptFunctionParametersAttribute)]);

			return CreateOrOpenScriptFunction((string)value, itemName, context.PropertyDescriptor.Name, parametersAttribute, scriptModule);
        }

		public static string CreateOrOpenScriptFunction(string stringValue, string itemName, string functionSuffix, ScriptFunctionParametersAttribute parametersAttribute, string scriptModule)
		{
			if (string.IsNullOrEmpty(stringValue))
			{
				if (string.IsNullOrEmpty(itemName))
				{
					System.Windows.Forms.MessageBox.Show("You must give this a name before creating scripts for it. Set the Name property on the main properties page.", "Name needed", System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning);
					return stringValue;
				}

				stringValue = itemName + "_" + functionSuffix;

				if (CreateScriptFunction != null)
				{
					string parameters = parametersAttribute.Parameters;
					CreateScriptFunction(scriptModule, stringValue, parameters);
				}
			}
			if (OpenScriptEditor != null)
			{
				OpenScriptEditor(scriptModule, stringValue);
			}
			return stringValue;
		}
    }
}
