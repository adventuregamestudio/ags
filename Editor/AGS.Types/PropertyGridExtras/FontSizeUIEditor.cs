using System;
using System.ComponentModel;
using System.Drawing.Design;

namespace AGS.Types
{
    public class FontSizeUIEditor : UITypeEditor
    {
        public delegate int FontImportGUIType(AGS.Types.Font font);
        public static FontImportGUIType FontSizeGUI;

        public FontSizeUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            int fontSize = (int)value;

            if (FontSizeGUI != null && context.Instance is Font)
            {
                fontSize = FontSizeGUI(context.Instance as Font);
            }

            return fontSize;
        }
    }
}
