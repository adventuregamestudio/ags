﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;

namespace AGS.Types
{
    public class BuildTargetUIEditor : StringListUIEditor
    {
        public BuildTargetUIEditor()
            : base()
        {
            editor.ItemCheck += new ItemCheckEventHandler(editor_ItemCheck);
        }

        void editor_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            CheckedListBox list = (sender as CheckedListBox);
            if (list == null) return;
            if ((list.Items[e.Index].ToString() == BuildTargetsInfo.DATAFILE_TARGET_NAME) &&
                (e.NewValue == CheckState.Unchecked))
            {
                e.NewValue = CheckState.Checked;
                MessageBox.Show("Data file cannot be deselected!", "Invalid selection", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            return base.EditValueHelper(context, provider, string.Format("{0}{1}{2}", BuildTargetsInfo.DATAFILE_TARGET_NAME,
                StringListUIEditor.Separators[0], value.ToString()),
                new List<string>(BuildTargetsInfo.GetAvailableBuildTargetNames()));
        }
    }
}
