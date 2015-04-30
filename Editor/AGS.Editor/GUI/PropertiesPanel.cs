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
            this.LoadColorTheme();              
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

        private void LoadColorTheme()
        {
            ColorTheme colorTheme = Factory.GUIController.UserColorTheme;
            colorTheme.Color_PropertyGrid(this.propertiesGrid);
            if(colorTheme is DraconianTheme)
            {
                //For reasons I'm unable to figure out, calling the combo box swap from code stops the text from rendering
                //However, calling the code lines directly form here makes it work, even though the code appears to be the same
                //colorTheme.Color_ComboBox(this.propertyObjectCombo, this);
                this.Controls.Remove(this.propertyObjectCombo);
                this.propertyObjectCombo = new DraconianComboBox(this.propertyObjectCombo);
                this.Controls.Add(this.propertyObjectCombo);
            }            
        }
    }
}
