﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;
using System.Windows.Forms.Design;

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

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("global/pane/background", c => BackColor = c);
            t.ComboBoxHelper(Controls, ref propertyObjectCombo, "properties-panel/combobox");
            t.PropertyGridHelper(propertiesGrid, "properties-panel/grid");
        }

        private void PropertiesPanel_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        public void ExpandAllGridItems()
        {
            propertiesGrid.ExpandAllGridItems();
        }

        public object SelectedObject
        {
            get { return propertiesGrid.SelectedObject; }
            set
            {
                propertiesGrid.SelectedObject = value;
            }
        }

        public object[] SelectedObjects
        { 
            get { return propertiesGrid.SelectedObjects; }
            set 
            {
                propertiesGrid.SelectedObjects = value;
            }
        }

        public GridItem SelectedGridItem
        {
            get { return propertiesGrid.SelectedGridItem; }
            set { propertiesGrid.SelectedGridItem = value; }
        }

        public PropertyTab SelectedTab
        {
            get { return propertiesGrid.SelectedTab; }
        }
    }
}
