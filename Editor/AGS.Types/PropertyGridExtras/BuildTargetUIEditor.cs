using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

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
            throw new NotImplementedException();
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            return base.EditValueHelper(context, provider, value, new FlagsUIEditorControl.ValueExclusionCheck(BuildTargetInfo.IsBuildTargetAvailable));
        }
    }
}
