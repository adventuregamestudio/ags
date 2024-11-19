using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;

namespace AGS.Types
{
    public class MultiLineStringUIEditor : UITypeEditor
    {
        public delegate String MultilineStringGUIType(String title, String text);
        public static MultilineStringGUIType MultilineStringGUI;

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            if (MultilineStringGUI == null)
                return value;

            String text = value as String;
            string title = "Edit text...";

            GUIControl ctrl = context.Instance as GUIControl;
            if (ctrl != null)
            {
                title = "Edit " + ctrl.Name + " text...";
            }

            GUIControl[] ctrls = context.Instance as GUIControl[];
            if (ctrls != null)
            {
                title = "Edit " + ctrls.Length.ToString() + " controls text...";
            }

            text = MultilineStringGUI(title, text);
            return text ?? value; // must return strictly original value if no change
        }
    }
}
