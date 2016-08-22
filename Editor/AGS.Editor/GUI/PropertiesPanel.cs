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

        private void PropertiesPanel_VisibleChanged(object sender, EventArgs e)
        {
            if (!this.Visible)
                return;
            // There is a strange bug that makes one tool button of the
            // PropertiesGrid dissapear after an object was assigned while the
            // grid was hidden from sight. Following code is a quick hack that
            // helps to reset control's toolbar after it becomes visible again.
            // TODO: remove this after the root of the problem is found
            // and fixed.
            if (propertiesGrid.SelectedObjects == null && propertiesGrid.SelectedObject == null)
                return;
            System.Windows.Forms.Design.PropertyTab tab = propertiesGrid.SelectedTab;
            if (propertiesGrid.SelectedObjects != null)
            {
                object[] objs = propertiesGrid.SelectedObjects;
                propertiesGrid.SelectedObjects = null;
                propertiesGrid.SelectedObjects = objs;
            }
            else if (propertiesGrid.SelectedObject != null)
            {
                object o = propertiesGrid.SelectedObject;
                propertiesGrid.SelectedObject = null;
                propertiesGrid.SelectedObject = o;
            }
            if (tab != null)
                SelectTabInPropertyGrid(tab.TabName);
        }

        public bool SelectTabInPropertyGrid(string tabName)
        {
            int tabIndex = 0;
            foreach (System.Windows.Forms.Design.PropertyTab propertyTab in propertiesGrid.PropertyTabs)
            {
                if (propertyTab.TabName == tabName)
                {
                    if (propertyTab != propertiesGrid.SelectedTab)
                    {
                        Hacks.SetSelectedTabInPropertyGrid(propertiesGrid, tabIndex);
                    }
                    return true;
                }
                tabIndex++;
            }
            return false;
        }
    }
}
