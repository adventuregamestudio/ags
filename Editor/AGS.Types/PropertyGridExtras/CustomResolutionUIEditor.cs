using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;

namespace AGS.Types
{
    public class CustomResolutionUIEditor : UITypeEditor
    {
        public delegate Size CustomResolutionGUIType(Size currentSize);
        public static CustomResolutionGUIType CustomResolutionSetGUI;

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            Size resolution = (Size)value;

            if (CustomResolutionSetGUI != null)
            {
                resolution = CustomResolutionSetGUI(resolution);
            }

            return resolution;
        }
    }
}
