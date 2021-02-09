using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;

namespace AGS.Types
{
    public class ColorUIEditor : UITypeEditor
    {
        public delegate Color ColorGUIType(Color color);
        public static ColorGUIType ColorGUI;

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.DropDown;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            Color color = (Color)value;
            if (ColorGUI != null)
            {
                color = ColorGUI(color);
            }
            return color;
        }
    }
}
