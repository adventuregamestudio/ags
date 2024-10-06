using System;
using System.ComponentModel;
using System.Drawing.Design;

namespace AGS.Types
{
    public class FontFileUIEditor : UITypeEditor
    {
        public delegate string FontImportGUIType(AGS.Types.Font font);
        public static FontImportGUIType FontFileGUI;

        public FontFileUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            string fontFile = (string)value;

            if (FontFileGUI != null && context.Instance is Font)
            {
                fontFile = FontFileGUI(context.Instance as Font);
            }

            return fontFile;
        }
    }
}
