using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;

namespace AGS.Types
{
    public class MultiLineStringUIEditor : UITypeEditor
    {
        public delegate String MultilineStringGUIType(String text);
        public static MultilineStringGUIType MultilineStringGUI;

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            String text = (String)value;
            if (MultilineStringGUI != null)
            {
                text = MultilineStringGUI(text);
            }
            return text ?? value; // must return strictly original value if no change
        }
    }
}
