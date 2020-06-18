using System;
using System.ComponentModel;
using System.Drawing.Design;

namespace AGS.Types
{
    public class ViewUIEditor : UITypeEditor
    {
        public delegate int ViewSelectionGUIType(int currentView);
        public static ViewSelectionGUIType ViewSelectionGUI;

        public ViewUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            int viewNumber = (int)value;

            if (ViewSelectionGUI != null)
            {
                viewNumber = ViewSelectionGUI(viewNumber);
            }

            return viewNumber;
        }
    }
}
