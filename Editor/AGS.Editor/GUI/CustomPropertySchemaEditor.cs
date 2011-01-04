using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class CustomPropertySchemaEditor : Form
    {
        private const string MENU_ITEM_ADD = "AddSchemaItem";
        private const string MENU_ITEM_EDIT = "EditSchemaItem";
        private const string MENU_ITEM_DELETE = "DeleteSchemaItem";

        private CustomPropertySchema _schema;

        public CustomPropertySchemaEditor(CustomPropertySchema schema)
        {
            InitializeComponent();
            _schema = schema;
        }

        private void RepopulateListView()
        {
            schemaList.Items.Clear();
            foreach (CustomPropertySchemaItem item in _schema.PropertyDefinitions)
            {
                ListViewItem newRow = new ListViewItem(item.Name);
                newRow.SubItems.Add(item.Description);
                newRow.SubItems.Add(item.Type.ToString());
                newRow.SubItems.Add(item.DefaultValue);
                newRow.SubItems.Add(item.AppliesToAsString);
                newRow.Tag = item;
                schemaList.Items.Add(newRow);
            }
        }

        private void CustomPropertySchemaEditor_Load(object sender, EventArgs e)
        {
            RepopulateListView();
        }

        private void schemaList_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location);
            }
        }

        private bool IsThereACustomPropertyWithThisName(string nameToCheckLowerCase)
        {
            foreach (CustomPropertySchemaItem item in _schema.PropertyDefinitions)
            {
                if (item.Name.ToLower() == nameToCheckLowerCase)
                {
                    MessageBox.Show("You already have a property with this name.", "Property already exists", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return true;
                }
            }
            return false;
        }

        private void EditOrAddItem(CustomPropertySchemaItem schemaItem)
        {
            bool isNewItem = false;
            if (schemaItem == null) 
            {
                schemaItem = new CustomPropertySchemaItem();
                schemaItem.Type = CustomPropertyType.Boolean;
                isNewItem = true;
            }
            CustomPropertySchemaItemEditor itemEditor = new CustomPropertySchemaItemEditor(schemaItem, isNewItem);
            if (itemEditor.ShowDialog() == DialogResult.OK)
            {
                if (isNewItem)
                {
                    if (!IsThereACustomPropertyWithThisName(schemaItem.Name.ToLower()))
                    {
                        _schema.PropertyDefinitions.Add(schemaItem);
                    }
                }
                RepopulateListView();
            }
            itemEditor.Dispose();
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = (ToolStripMenuItem)sender;
            CustomPropertySchemaItem schemaItem = (CustomPropertySchemaItem)menuItem.Owner.Tag;
            if (menuItem.Name == MENU_ITEM_ADD)
            {
                EditOrAddItem(null);
            }
            else if (menuItem.Name == MENU_ITEM_EDIT)
            {
                EditOrAddItem(schemaItem);
            }
            else if (menuItem.Name == MENU_ITEM_DELETE)
            {
                if (MessageBox.Show("Are you sure you want to delete this entry from the schema? It will be removed from all items in the game that currently have this property set.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    _schema.PropertyDefinitions.Remove(schemaItem);
                    RepopulateListView();
                }
            }
        }

        private void ShowContextMenu(Point location)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (schemaList.SelectedItems.Count == 1)
            {
                menu.Tag = schemaList.SelectedItems[0].Tag;
                menu.Items.Add(new ToolStripMenuItem("Edit property...", null, onClick, MENU_ITEM_EDIT));
                menu.Items.Add(new ToolStripMenuItem("Delete property...", null, onClick, MENU_ITEM_DELETE));
                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Add new property...", null, onClick, MENU_ITEM_ADD));

            menu.Show(schemaList, location);
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void schemaList_ItemActivate(object sender, EventArgs e)
        {
            if (schemaList.SelectedItems.Count == 1)
            {
                EditOrAddItem((CustomPropertySchemaItem)schemaList.SelectedItems[0].Tag);
            }
        }
    }
}