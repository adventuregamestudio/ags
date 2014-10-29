using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;

// Borrowed from <http://www.ozcandegirmenci.com/post/2008/08/How-to-permit-multiple-selections-for-Enum-properties.aspx>.
// Copyright 2008 Ozcan DEGIRMENCI.

namespace AGS.Types
{
    public partial class FlagsUIEditorControl : UserControl
    {
        // CheckedListBox control moved to designer code
        public event ItemCheckEventHandler ItemCheck;
        FlagsUIEditor Editor = null;
        IWindowsFormsEditorService _Service = null;
        object _Value;
        long leftOver = 0;
        bool cancelFlag = false;

        public FlagsUIEditorControl()
        {
            InitializeComponent();
        }

        public FlagsUIEditorControl(FlagsUIEditor editor)
            : this()
        {
            Editor = editor;
        }

        public delegate bool ValueExclusionCheck(Enum value);

        public void Begin(IWindowsFormsEditorService service, object value)
        {
            Begin(service, value, null);
        }

        // begin edit operation
        public void Begin(IWindowsFormsEditorService service, object value, ValueExclusionCheck valueCheck)
        {
            _Service = service;
            lvwItems.Items.Clear();

            Type enumType = value.GetType();
            Array values = Enum.GetValues(enumType);

            // prepare list
            long current = Convert.ToInt64(value);
            for (int i = 0; i < values.Length; i++)
            {
                if ((valueCheck != null) && (!valueCheck((Enum)values.GetValue(i)))) continue;
                long val = Convert.ToInt64(values.GetValue(i));
                bool check = false;
                if (val == 0)
                    check = ((current == 0) || (Editor.AlwaysIncludeZero));
                else
                {
                    check = ((current & val) == val);
                    if (check)
                        current &= ~val;
                }

                lvwItems.Items.Add(new EnumValueContainer(enumType, val), check);
            }
            leftOver = current;
            _Value = value;
        }

        // end edit operation
        public void End()
        {
            cancelFlag = false;
            _Service = null;
            _Value = 0;
            leftOver = 0;
        }

        // value which will be calculated from the checked items list
        public object Value
        {
            get
            {
                // if cancel flag set, return original value
                if (cancelFlag)
                    return _Value;

                long value = 0;
                for (int i = 0; i < lvwItems.CheckedItems.Count; i++)
                {
                    EnumValueContainer container = lvwItems.CheckedItems[i] as EnumValueContainer;
                    value |= Convert.ToInt64(container.Value);
                }

                return value | leftOver;
            }
        }

        protected override bool ProcessDialogKey(Keys keyData)
        {
            if (((keyData & Keys.KeyCode) == Keys.Return)
                && ((keyData & (Keys.Alt | Keys.Control)) == Keys.None))
            {
                _Service.CloseDropDown();
                return true;
            }
            if (((keyData & Keys.KeyCode) == Keys.Escape)
                && ((keyData & (Keys.Alt | Keys.Control)) == Keys.None))
            {
                cancelFlag = true;
                _Service.CloseDropDown();
                return true;
            }

            return base.ProcessDialogKey(keyData);
        }

        // container class for the enum values
        private class EnumValueContainer
        {
            Type _Type;
            object _Value = 0;

            public EnumValueContainer(Type type, object value)
            {
                _Type = type;
                _Value = value;
            }

            // return the Name of the Enum member according to its value
            public override string ToString()
            {
                return Enum.GetName(_Type, _Value);
            }

            public object Value
            {
                get { return _Value; }
            }
        }

        void lvwItems_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
        {
            if (ItemCheck != null) ItemCheck(sender, e);
        }
    }
}
