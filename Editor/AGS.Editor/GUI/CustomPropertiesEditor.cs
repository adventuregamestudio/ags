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
    public partial class CustomPropertiesEditor : Form
    {
        private CustomProperties _properties;
        private CustomPropertySchema _schema;
        private CustomPropertyAppliesTo _showPropertiesThatApplyTo;

        public CustomPropertiesEditor(CustomPropertySchema schema, CustomProperties properties, CustomPropertyAppliesTo showPropertiesThatApplyTo)
        {
            InitializeComponent();
            _properties = properties;
            _schema = schema;
            _showPropertiesThatApplyTo = showPropertiesThatApplyTo;

            propertyGrid.SelectedObject = new CustomPropertyBag(schema, _properties, _showPropertiesThatApplyTo);
        }

        private void btnEditSchema_Click(object sender, EventArgs e)
        {
            CustomPropertySchemaEditor schemaEditor = new CustomPropertySchemaEditor(_schema);
            schemaEditor.ShowDialog();
            schemaEditor.Dispose();
            propertyGrid.SelectedObject = new CustomPropertyBag(_schema, _properties, _showPropertiesThatApplyTo);
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}