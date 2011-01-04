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
        public delegate void OpenScriptEditorHandler(bool isGlobalScript, string functionName);
        public static OpenScriptEditorHandler OpenScriptEditor;
        public delegate void CreateScriptFunctionHandler(bool isGlobalScript, string functionName, string parameters);
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
            bool isGlobalScript = false;
			int maxLength = 50;
            string itemName = string.Empty;
            if (context.Instance is GUI)
            {
                itemName = ((GUI)context.Instance).Name;
				maxLength = NormalGUI.MAX_EVENT_HANDLER_LENGTH;
                isGlobalScript = true;
            }
            else if (context.Instance is GUIControl)
            {
                itemName = ((GUIControl)context.Instance).Name;
				maxLength = GUIControl.MAX_EVENT_HANDLER_LENGTH;
                isGlobalScript = true;
            }
            else if (context.Instance is InventoryItem)
            {
                itemName = ((InventoryItem)context.Instance).Name;
                isGlobalScript = true;
            }
            else if (context.Instance is Character)
            {
                itemName = ((Character)context.Instance).ScriptName;
                isGlobalScript = true;
            }
            else if (context.Instance is Room)
            {
                itemName = "room";
            }
            else if (context.Instance is RoomHotspot)
            {
                itemName = ((RoomHotspot)context.Instance).Name;
            }
            else if (context.Instance is RoomObject)
            {
                itemName = ((RoomObject)context.Instance).Name;
            }
            else if (context.Instance is RoomRegion)
            {
                itemName = "region" + ((RoomRegion)context.Instance).ID;
            }

			ScriptFunctionParametersAttribute parametersAttribute = ((ScriptFunctionParametersAttribute)context.PropertyDescriptor.Attributes[typeof(ScriptFunctionParametersAttribute)]);

			return CreateOrOpenScriptFunction((string)value, itemName, context.PropertyDescriptor.Name, parametersAttribute, isGlobalScript, maxLength);
        }

		public static string CreateOrOpenScriptFunction(string stringValue, string itemName, string functionSuffix, ScriptFunctionParametersAttribute parametersAttribute, bool isGlobalScript, int maxLength)
		{
			if (string.IsNullOrEmpty(stringValue))
			{
				if (string.IsNullOrEmpty(itemName))
				{
					System.Windows.Forms.MessageBox.Show("You must give this a name before creating scripts for it. Set the Name property on the main properties page.", "Name needed", System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning);
					return stringValue;
				}

				stringValue = itemName + "_" + functionSuffix;

				if (stringValue.Length > maxLength)
				{
					stringValue = stringValue.Substring(0, maxLength);
				}

				if (CreateScriptFunction != null)
				{
					string parameters = parametersAttribute.Parameters;
					CreateScriptFunction(isGlobalScript, stringValue, parameters);
				}
			}
			if (OpenScriptEditor != null)
			{
				OpenScriptEditor(isGlobalScript, stringValue);
			}
			return stringValue;
		}
    }
}
