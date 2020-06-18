using System;
using System.ComponentModel;
using System.Drawing.Design;

namespace AGS.Types
{
    public class SpriteSelectUIEditor : UITypeEditor
    {
        public delegate int SpriteSelectionGUIType(int currentSprite);
        public static SpriteSelectionGUIType SpriteSelectionGUI;

        public SpriteSelectUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            int spriteNumber = (int)value;

            if (SpriteSelectionGUI != null)
            {
                spriteNumber = SpriteSelectionGUI(spriteNumber);
            }

            return spriteNumber;
        }
    }
}
