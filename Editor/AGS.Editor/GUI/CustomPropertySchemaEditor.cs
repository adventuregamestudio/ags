using AGS.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public enum CustomPropertyDefChangeType
    {
        None,
        Add,
        Edit,
        Remove
    }

    public struct CustomPropertyDefChange
    {
        public CustomPropertyDefChangeType Type;
        public string OriginalName;
        public string NewName;

        public CustomPropertyDefChange(CustomPropertyDefChangeType type, string originalName, string newName)
        {
            Type = type;
            OriginalName = originalName;
            NewName = newName;
        }
    }

    public partial class CustomPropertySchemaEditor : Form
    {
        private const string MENU_ITEM_ADD = "AddSchemaItem";
        private const string MENU_ITEM_EDIT = "EditSchemaItem";
        private const string MENU_ITEM_DELETE = "DeleteSchemaItem";

        private CustomPropertySchema _schema;
        private CustomPropertySchema _srcSchema;
        private List<CustomPropertyDefChange> _schemaChanges = new List<CustomPropertyDefChange>();

        public CustomPropertySchemaEditor(CustomPropertySchema schema)
        {
            InitializeComponent();
            _srcSchema = schema;
            _schema = new CustomPropertySchema(schema);
        }

        public List<CustomPropertyDefChange> SchemaChanges
        {
            get { return _schemaChanges; }
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
                newRow.SubItems.Add((item.Type == CustomPropertyType.Text) ? (item.Translated ? "Yes" : "No") : "-");
                newRow.Tag = item;
                schemaList.Items.Add(newRow);
            }
            schemaList.Sort();
        }

        private void CustomPropertySchemaEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                var config = GUIController.Instance.WindowConfig;
                ConfigUtils.ReadFormPosition(config, "CustomPropertySchemaEditor", this);
            }

            RepopulateListView();
        }

        private void CustomPropertySchemaEditor_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (!DesignMode)
            {
                var config = GUIController.Instance.WindowConfig;
                ConfigUtils.WriteFormPosition(config, "CustomPropertySchemaEditor", this);
            }
        }

        private void schemaList_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location);
            }
        }

        private void EditOrAddItem(CustomPropertySchemaItem schemaItem)
        {
            bool isNewItem = false;
            string oldName = null;
            if (schemaItem == null) 
            {
                schemaItem = new CustomPropertySchemaItem();
                schemaItem.Type = CustomPropertyType.Boolean;
                isNewItem = true;
            }
            else
            {
                oldName = schemaItem.Name;
            }

            CustomPropertySchemaItemEditor itemEditor = new CustomPropertySchemaItemEditor(schemaItem, isNewItem, _schema);
            if (itemEditor.ShowDialog() == DialogResult.OK)
            {
                if (isNewItem)
                {
                    _schema.PropertyDefinitions.Add(schemaItem);
                    _schemaChanges.Add(new CustomPropertyDefChange(CustomPropertyDefChangeType.Add, null, schemaItem.Name));
                }
                else
                {
                    _schemaChanges.Add(new CustomPropertyDefChange(CustomPropertyDefChangeType.Edit, oldName, schemaItem.Name));
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
                if (MessageBox.Show("Are you sure you want to delete this entry from the schema? It will be removed from all objects in the game that currently have this property set.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    _schema.PropertyDefinitions.Remove(schemaItem);
                    _schemaChanges.Add(new CustomPropertyDefChange(CustomPropertyDefChangeType.Remove, schemaItem.Name, null));
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
            _srcSchema.CopyFrom(_schema);
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
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