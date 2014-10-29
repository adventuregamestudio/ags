using System;
using System.ComponentModel;
using System.Drawing.Design;
using System.Windows.Forms.Design;

// Borrowed from <http://www.ozcandegirmenci.com/post/2008/08/How-to-permit-multiple-selections-for-Enum-properties.aspx>.
// Copyright 2008 Ozcan DEGIRMENCI.

namespace AGS.Types
{
    public class FlagsUIEditor : UITypeEditor
    {
        protected FlagsUIEditorControl editor = null;

        public FlagsUIEditor()
        {
            editor = new FlagsUIEditorControl(this);
        }

        // our editor is a DropDown editor
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.DropDown;
        }

        public virtual bool AlwaysIncludeZero
        {
            get
            {
                return false;
            }
        }

        protected object EditValueHelper(ITypeDescriptorContext context,
            IServiceProvider provider, object value,
            FlagsUIEditorControl.ValueExclusionCheck valueCheck)
        {
            // if value is not an enum than we can not edit it
            if (!(value is Enum))
                throw new Exception("Value is not supported");

            // try to figure out that is this a Flags enum or not ?
            Type enumType = value.GetType();
            object[] attributes = enumType.GetCustomAttributes(typeof(FlagsAttribute), true);
            if (attributes.Length == 0)
                throw new Exception("Editing enum hasn't got Flags attribute");

            // check the underlying type
            Type type = Enum.GetUnderlyingType(value.GetType());
            if (type != typeof(byte) && type != typeof(sbyte)
                   && type != typeof(short) && type != typeof(ushort)
                   && type != typeof(int) && type != typeof(uint))
                return value;

            if (provider != null)
            {
                // use windows forms editor service to show drop down
                IWindowsFormsEditorService edSvc = provider.GetService(
                                typeof(IWindowsFormsEditorService))
                              as IWindowsFormsEditorService;
                if (edSvc == null)
                    return value;
                //if (editor == null)
                //    editor = new FlagsUIEditorControl(this);
                // prepare list
                editor.Begin(edSvc, value, valueCheck);
                // show drop down now
                edSvc.DropDownControl(editor);
                // now we take the result
                value = editor.Value;
                // reset editor
                editor.End();
            }
            return Convert.ChangeType(value, type);
        }

        public override object EditValue(ITypeDescriptorContext context,
                                         IServiceProvider provider, object value)
        {
            return EditValueHelper(context, provider, value, null);
        }
    }
}
