using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class ToolBarManager
    {
        private ToolStrip _toolBar;
        private List<ToolStripButton> _buttonsForCurrentPane = new List<ToolStripButton>();
        private ToolStripSeparator _paneSpecificSeparator = new ToolStripSeparator();

        internal ToolBarManager(ToolStrip toolBar)
        {
            _toolBar = toolBar;
        }

        private void ToolbarEventHandler(object sender, EventArgs e)
        {
            ToolStripButton button = (ToolStripButton)sender;
            if (button.Tag is IEditorComponent)
            {
                IEditorComponent component = (IEditorComponent)button.Tag;
                component.CommandClick(button.Name);
            }
            else
            {
                ContentDocument document = (ContentDocument)button.Tag;
                document.Control.CommandClick(button.Name);
            }
        }

        public void AddGlobalItem(IEditorComponent component, MenuCommand command)
        {
            ToolStripButton button = new ToolStripButton(string.Empty, Factory.GUIController.ImageList.Images[command.IconKey], new EventHandler(ToolbarEventHandler), command.ID);
            button.Tag = component;
            button.ToolTipText = command.Name;
            _toolBar.Items.Add(button);
        }

        public void AddGlobalItems(IEditorComponent component, List<MenuCommand> commands)
        {
            if (_toolBar.Items.Count > 0)
            {
                _toolBar.Items.Add(new ToolStripSeparator());
            }

            foreach (MenuCommand command in commands)
            {
                AddGlobalItem(component, command);
            }
        }

        public void UpdateItemEnabledStates(List<MenuCommand> commands)
        {
            foreach (MenuCommand command in commands)
            {
                foreach (ToolStripItem button in _toolBar.Items)
                {
                    if (button.Name == command.ID)
                    {
                        button.Enabled = command.Enabled;
                    }
                }
            }
        }

        public void RefreshCurrentPane()
        {
            ShowPane(Factory.GUIController.ActivePane);
        }

        public void ShowPane(ContentDocument doc)
        {
            RemoveItemsFromLastPane();

            if ((doc == null) || (doc.ToolbarCommands == null))
            {
                return;
            }

            if (_toolBar.Items.Count > 0)
            {
                _toolBar.Items.Add(_paneSpecificSeparator);
            }

            foreach (MenuCommand command in doc.ToolbarCommands)
            {
                ToolStripButton button = new ToolStripButton(string.Empty, Factory.GUIController.ImageList.Images[command.IconKey], new EventHandler(ToolbarEventHandler), command.ID);
                button.Tag = doc;
                button.ToolTipText = command.Name;
                button.Enabled = command.Enabled;
                if (command.Checked)
                {
                    button.CheckState = CheckState.Checked;
                }
                _toolBar.Items.Add(button);
                _buttonsForCurrentPane.Add(button);
            }
        }

        private void RemoveItemsFromLastPane()
        {
            if (_toolBar.Items.Contains(_paneSpecificSeparator))
            {
                _toolBar.Items.Remove(_paneSpecificSeparator);
            }
            foreach (ToolStripButton button in _buttonsForCurrentPane)
            {
                _toolBar.Items.Remove(button);
            }
            _buttonsForCurrentPane.Clear();
        }
    }
}
