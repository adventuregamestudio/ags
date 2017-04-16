using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing.Design;
using System.Text;
using System.Windows.Forms.Design;

// Based on an idea by Ozcan Degirmenci:
// <http://www.ozcandegirmenci.com/post/2008/08/How-to-permit-multiple-selections-for-Enum-properties.aspx>

namespace AGS.Types
{
    public class StringListUIEditor : UITypeEditor
    {
        protected StringListUIEditorControl editor = null;

        public static string[] Separators
        {
            get
            {
                return new string[] { ", ", "," };
            }
        }

        public StringListUIEditor()
        {
            editor = new StringListUIEditorControl();
        }

        public override UITypeEditorEditStyle GetEditStyle(System.ComponentModel.ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.DropDown;
        }

        protected string EditValueHelper(ITypeDescriptorContext context, IServiceProvider provider,
            string value, IList<string> valueList)
        {
            if ((value == null) || (valueList == null))
            {
                throw new ArgumentNullException();
            }
            if (provider != null)
            {
                IWindowsFormsEditorService editorService = provider.GetService(typeof(IWindowsFormsEditorService))
                    as IWindowsFormsEditorService;
                if (editorService == null) return value;
                // prepare list
                editor.Begin(editorService, value, valueList);
                // show drop-down now
                editorService.DropDownControl(editor);
                // now we take the result
                value = editor.Value;
                // reset editor
                editor.End();
            }
            return value;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            return EditValueHelper(context, provider, value.ToString(), new List<string>());
        }
    }
}
