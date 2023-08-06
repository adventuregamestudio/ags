using System;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;

namespace AGS.Types
{
    public class AudioClipSourceFileUIEditor : UITypeEditor
    {
        public delegate AudioClip AudioClipSourceFileGUIType(AudioClip audioClip);
        public static AudioClipSourceFileGUIType AudioClipSourceFileGUI;

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            AudioClip audioClip = context.Instance as AudioClip;

            if (AudioClipSourceFileGUI != null && audioClip != null)
            {
                audioClip = AudioClipSourceFileGUI(audioClip);
                return audioClip.SourceFileName;
            }
            return value;
        }
    }
}