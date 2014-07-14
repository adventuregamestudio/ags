using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class PropertiesPanel : DockContent
    {
        public PropertiesPanel()
        {
            InitializeComponent();
        }

        public event PropertyValueChangedEventHandler PropertyValueChanged
        {
            add { propertiesGrid.PropertyValueChanged += value; }
            remove { propertiesGrid.PropertyValueChanged -= value; }
        }

        public event EventHandler SelectedIndexChanged
        {
            add { propertyObjectCombo.SelectedIndexChanged += value; }
            remove { propertyObjectCombo.SelectedIndexChanged -= value; }
        }
    }
}
