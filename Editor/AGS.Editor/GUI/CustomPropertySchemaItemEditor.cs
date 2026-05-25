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
        private CustomPropertySchema _schema;
        private CustomPropertySchemaItem _itemToEdit;
        private CustomPropertySchemaItem _copyOfItem;

        public CustomPropertySchemaItemEditor(CustomPropertySchemaItem item, bool isNewItem, CustomPropertySchema schema)
        {
            _schema = schema;
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
            chkAudioClips.DataBindings.Add("Checked", _copyOfItem, "AppliesToAudioClips", true, DataSourceUpdateMode.OnPropertyChanged);
            chkDialogs.DataBindings.Add("Checked", _copyOfItem, "AppliesToDialogs", true, DataSourceUpdateMode.OnPropertyChanged);
            chkGUIs.DataBindings.Add("Checked", _copyOfItem, "AppliesToGUIs", true, DataSourceUpdateMode.OnPropertyChanged);
            chkGUIControls.DataBindings.Add("Checked", _copyOfItem, "AppliesToGUIControls", true, DataSourceUpdateMode.OnPropertyChanged);
            chkRegions.DataBindings.Add("Checked", _copyOfItem, "AppliesToRegions", true, DataSourceUpdateMode.OnPropertyChanged);
            chkWalkareas.DataBindings.Add("Checked", _copyOfItem, "AppliesToWalkableAreas", true, DataSourceUpdateMode.OnPropertyChanged);
            chkTranslated.DataBindings.Add("Checked", _copyOfItem, "Translated", true, DataSourceUpdateMode.OnPropertyChanged);
            cmbType.SelectedIndex = ((int)_copyOfItem.Type) - 1;

            txtName.Enabled = isNewItem;
            btnRename.Enabled = !isNewItem;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            CustomPropertyType propertyType = (CustomPropertyType)(cmbType.SelectedIndex + 1);
            if (_copyOfItem.Name == string.Empty)
            {
                MessageBox.Show("You must enter a name for the new property.", "Name missing", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                txtName.Focus();
                return;
            }
            // Test if there is no item of this name, except for the item which we current edit
            if (_schema.PropertyDefinitions.Find(pd => pd != _itemToEdit && pd.Name.ToLowerInvariant() == _copyOfItem.Name.ToLowerInvariant()) != null)
            {
                MessageBox.Show("You already have a property with this name.", "Property already exists", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                txtName.Focus();
                return;
            }
            if (propertyType == CustomPropertyType.Boolean)
            {
                if ((_copyOfItem.DefaultValue != string.Empty) &&
                    (_copyOfItem.DefaultValue != "1") &&
                    (_copyOfItem.DefaultValue != "0"))
                {
                    MessageBox.Show("The default value for a Boolean item must be 0 or 1.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    txtDefaultValue.Focus();
                    return;
                }
            }
            if (propertyType == CustomPropertyType.Number)
            {
                int result;
                if (!Int32.TryParse(_copyOfItem.DefaultValue, out result))
                {
                    MessageBox.Show("The default value for a Number item must be a valid integer.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    txtDefaultValue.Focus();
                    return;
                }
            }
            _copyOfItem.Type = propertyType;

            _itemToEdit.CopyFrom(_copyOfItem);
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void CustomPropertySchemaItemEditor_FormClosed(object sender, FormClosedEventArgs e)
        {
            // refresh property grid, a property may have been added, changed or removed
            Factory.GUIController.RefreshPropertyGrid();
        }

        private void cmbType_SelectedIndexChanged(object sender, EventArgs e)
        {
            chkTranslated.Enabled = ((CustomPropertyType)(cmbType.SelectedIndex + 1)) == CustomPropertyType.Text;
        }

        private void btnRename_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show("Are you sure you want to rename this property? Editor will automatically replace it in all objects in the game that currently have this property set (including rooms)."
                + $"{Environment.NewLine}But you will have to fix all of its uses in script by hand!", "Confirm rename", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                txtName.Enabled = true;
                btnRename.Enabled = false;
            }
        }
    }
}