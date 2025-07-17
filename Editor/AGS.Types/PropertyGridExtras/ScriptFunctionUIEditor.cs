using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    public class ScriptFunctionUIEditor : UITypeEditor
    {
        public delegate void OpenScriptFunctionHandler(OpenScriptFunctionArgs args);
        public static OpenScriptFunctionHandler OpenScriptFunction;
        public delegate bool CreateScriptFunctionHandler(CreateScriptFunctionArgs args);
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
                scriptModule = ((GUI)context.Instance).ScriptModule;
            }
            else if (context.Instance is GUIControl)
            {
                itemName = ((GUIControl)context.Instance).Name;
                GUI gui = ((GUIControl)context.Instance).Parent;
                if (gui != null)
                    scriptModule = gui.ScriptModule;
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
                    MessageBox.Show("You must give this object a name before creating scripts for it. Set the Name property on the main properties page.", "Name needed", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return stringValue;
                }

                if (CreateScriptFunction != null)
                {
                    string newStringValue = itemName + "_" + functionSuffix;
                    if (CreateScriptFunction.Invoke(new CreateScriptFunctionArgs(scriptModule, newStringValue, parametersAttribute.Parameters)))
                    {
                        stringValue = newStringValue;
                    }
                    else
                    {
                        return stringValue;
                    }
                }
            }

            OpenScriptFunction?.Invoke(new OpenScriptFunctionArgs(scriptModule, stringValue, parametersAttribute.Parameters, true));
            return stringValue;
		}
    }
}
