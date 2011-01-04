using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class CustomPropertySchemaItemEditor : Form
    {
        private CustomPropertySchemaItem _itemToEdit;
        private CustomPropertySchemaItem _copyOfItem;

        public CustomPropertySchemaItemEditor(CustomPropertySchemaItem item, bool isNewItem)
        {
            _itemToEdit = item;
            _copyOfItem = (CustomPropertySchemaItem)item.Clone();
            InitializeComponent();

            txtName.DataBindings.Add("Text", _copyOfItem, "Name", true, DataSourceUpdateMode.OnPropertyChanged);
            txtDescription.DataBindings.Add("Text", _copyOfItem, "Description", true, DataSourceUpdateMode.OnPropertyChanged);
            txtDefaultValue.DataBindings.Add("Text", _copyOfItem, "DefaultValue", true, DataSourceUpdateMode.OnPropertyChanged);
            chkCharacters.DataBindings.Add("Checked", _copyOfItem, "AppliesToCharacters", true, DataSourceUpdateMode.OnPropertyChanged);
            chkHotspots.DataBindings.Add("Checked", _copyOfItem, "AppliesToHotspots", true, DataSourceUpdateMode.OnPropertyChanged);
            chkInventory.DataBindings.Add("Checked", _copyOfItem, "AppliesToInvItems", true, DataSourceUpdateMode.OnPropertyChanged);
            chkObjects.DataBindings.Add("Checked", _copyOfItem, "AppliesToObjects", true, DataSourceUpdateMode.OnPropertyChanged);
            chkRooms.DataBindings.Add("Checked", _copyOfItem, "AppliesToRooms", true, DataSourceUpdateMode.OnPropertyChanged);
            cmbType.SelectedIndex = ((int)_copyOfItem.Type) - 1;

            if (!isNewItem)
            {
                txtName.Enabled = false;
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            CustomPropertyType propertyType = (CustomPropertyType)(cmbType.SelectedIndex + 1);
            if (_copyOfItem.Name == string.Empty)
            {
                MessageBox.Show("You must enter a name for the new property.", "Name missing", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            if (propertyType == CustomPropertyType.Boolean)
            {
                if ((_copyOfItem.DefaultValue != string.Empty) &&
                    (_copyOfItem.DefaultValue != "1") &&
                    (_copyOfItem.DefaultValue != "0"))
                {
                    MessageBox.Show("The default value for a Boolean item must be 0 or 1.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
            }
            if (propertyType == CustomPropertyType.Number)
            {
                int result;
                if (!Int32.TryParse(_copyOfItem.DefaultValue, out result))
                {
                    MessageBox.Show("The default value for a Number item must be a valid integer.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
            }
            _itemToEdit.Name = _copyOfItem.Name;
            _itemToEdit.Description = _copyOfItem.Description;
            _itemToEdit.DefaultValue = _copyOfItem.DefaultValue;
            _itemToEdit.Type = propertyType;
            _itemToEdit.AppliesToCharacters = _copyOfItem.AppliesToCharacters;
            _itemToEdit.AppliesToHotspots = _copyOfItem.AppliesToHotspots;
            _itemToEdit.AppliesToInvItems = _copyOfItem.AppliesToInvItems;
            _itemToEdit.AppliesToObjects = _copyOfItem.AppliesToObjects;
            _itemToEdit.AppliesToRooms = _copyOfItem.AppliesToRooms;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }
    }
}