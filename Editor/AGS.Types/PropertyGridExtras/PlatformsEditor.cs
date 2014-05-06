using System;
using System.ComponentModel;
using System.Windows.Forms;

namespace AGS.Types
{
    public class PlatformsEditor : FlagsEditor
    {
        public PlatformsEditor() : base()
        {
            editor.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(editor_ItemCheck);
        }

        void editor_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
        {
            CheckedListBox list = (sender as CheckedListBox);
            if (list == null) return;
            if ((list.Items[e.Index].ToString() == "Windows") && (e.NewValue == CheckState.Unchecked))
            {
                e.NewValue = CheckState.Checked;
                MessageBox.Show("Windows platform cannot be deselected!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            /*if (value) AGSEditor.Instance.ExtraOutputCreationStep += new AGSEditor.ExtraOutputCreationStepHandler(AGS.Editor.Components.BuildLinuxComponent.Instance.BuildForLinux);
            else
            {
                AGSEditor.Instance.ExtraOutputCreationStep -= new AGSEditor.ExtraOutputCreationStepHandler(AGS.Editor.Components.BuildLinuxComponent.Instance.BuildForLinux);
                string dataDir = AGS.Editor.Components.BuildLinuxComponent.Instance.LinuxDataDirectory;
                if (dataDir != null) // dir will be null when first opening editor
                {
                    Utilities.DeleteFileIfExists(System.IO.Path.Combine(dataDir, AGSEditor.Instance.BaseGameFileName + ".exe"));
                }
            }*/
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            return base.EditValueHelper(context, provider, value, new FlagsEditorControl.ValueExclusionCheck(AGS.Types.Targets.IsPlatformAvailable));
        }
    }
}
