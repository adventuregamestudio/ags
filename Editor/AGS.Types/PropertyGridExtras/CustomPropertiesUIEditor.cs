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
    public class CustomPropertiesUIEditor : UITypeEditor
    {
        public delegate void CustomPropertiesGUIType(CustomProperties currentProperties, object objectThatHasProperties);
        public static CustomPropertiesGUIType CustomPropertiesGUI;

        public CustomPropertiesUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            CustomProperties properties = (CustomProperties)value;

            if (CustomPropertiesGUI != null)
            {
                CustomPropertiesGUI(properties, context.Instance);

                if (context.Instance is IChangeNotification)
                {
                    ((IChangeNotification)context.Instance).ItemModified();
                }
            }

            return properties;
        }
    }
}
