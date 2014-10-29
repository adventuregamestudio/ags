using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
    public class BuildTargetUIEditor : FlagsUIEditor
    {
        public BuildTargetUIEditor()
            : base()
        {
            editor.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(editor_ItemCheck);
        }

        void editor_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
        {
            CheckedListBox list = (sender as CheckedListBox);
            if (list == null) return;
            if ((list.Items[e.Index].ToString() == "DataFile") && (e.NewValue == CheckState.Unchecked))
            {
                e.NewValue = CheckState.Checked;
                MessageBox.Show("Data file cannot be deselected!", "Invalid selection", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        public override bool AlwaysIncludeZero
        {
            get
            {
                return true; // here BuildTargetPlatform.DataFile is zero, always include it as checked
            }
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            return base.EditValueHelper(context, provider, value, new FlagsUIEditorControl.ValueExclusionCheck(BuildTargetInfo.IsBuildTargetAvailable));
        }
    }
}
