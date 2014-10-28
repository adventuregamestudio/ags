using System;
using System.ComponentModel;
using System.Windows.Forms;

namespace AGS.Types
{
    public class PlatformsEditor : FlagsUIEditor
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
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            //return base.EditValueHelper(context, provider, value, new FlagsUIEditorControl.ValueExclusionCheck(AGS.Types.Targets.IsPlatformAvailable));
            return false;
        }
    }
}
