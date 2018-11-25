using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor.Panes.Room
{
    public partial class RoomNodeControl : UserControl
    {        
        public RoomNodeControl()
        {
            InitializeComponent();
            this.label.Click += RoomNodeControl_Click;
            this.Click += RoomNodeControl_Click;
            this.lockedCheckbox.CheckedChanged += lockedCheckbox_CheckedChanged;
            this.visibleCheckbox.CheckedChanged += visibleCheckbox_CheckedChanged;
            this.MouseEnter += RoomNodeControl_MouseEnter;
            this.MouseLeave += RoomNodeControl_MouseLeave;
            this.label.MouseEnter += RoomNodeControl_MouseEnter;
            this.lockedCheckbox.MouseEnter += lockedCheckbox_MouseEnter;
            this.visibleCheckbox.MouseEnter += visibleCheckbox_MouseEnter;
            this.label.MouseLeave += RoomNodeControl_MouseLeave;
            this.lockedCheckbox.MouseLeave += lockedCheckbox_MouseLeave;
            this.visibleCheckbox.MouseLeave += visibleCheckbox_MouseLeave;                        
        }

        public event EventHandler OnNodeSelected;
        public event EventHandler OnIsVisibleChanged { add { visibleCheckbox.CheckedChanged += value; } remove { visibleCheckbox.CheckedChanged -= value; } }
        public event EventHandler OnIsLockedChanged { add { lockedCheckbox.CheckedChanged += value; } remove { lockedCheckbox.CheckedChanged -= value; } }
        
        public string DisplayName { get { return label.Text; } set { label.Text = value; } }

        public bool IsVisible 
        { 
            get { return visibleCheckbox.Checked; }
            set
            {
                visibleCheckbox.Checked = value;
                visibleCheckbox_CheckedChanged(this, null);
            }
        }

        public bool IsLocked 
        { 
            get { return lockedCheckbox.Checked; } 
            set 
            { 
                lockedCheckbox.Checked = value;
                lockedCheckbox_CheckedChanged(this, null);
            } 
        }

        public ToolStripItem Host { get; set; }

        public void HideCheckBoxes(bool hideLocked)
        {
            visibleCheckbox.Visible = false;
            if (hideLocked)
            {
                label.Location = new Point(lockedCheckbox.Left, label.Top);
                lockedCheckbox.Visible = false;                
            }
            else
            {
                label.Location = new Point(visibleCheckbox.Left, label.Top);                
            }
        }

        private void RoomNodeControl_Click(object sender, EventArgs e)
        {
            Parent.Hide();
            if (OnNodeSelected != null) OnNodeSelected(Host, e);
        }
        
        private void visibleCheckbox_MouseEnter(object sender, EventArgs e)
        {
            OnCheckBoxMouseEnter(visibleCheckbox);
        }

        private void visibleCheckbox_MouseLeave(object sender, EventArgs e)
        {
            OnCheckBoxMouseLeave(visibleCheckbox);
        }

        private void lockedCheckbox_MouseEnter(object sender, EventArgs e)
        {
            OnCheckBoxMouseEnter(lockedCheckbox);
        }

        private void lockedCheckbox_MouseLeave(object sender, EventArgs e)
        {
            OnCheckBoxMouseLeave(lockedCheckbox);
        }

        private void OnCheckBoxMouseEnter(CheckBox checkBox)
        {
            RoomNodeControl_MouseEnter(this, null);
            if (checkBox.Checked) checkBox.BackColor = Color.Honeydew;
            else checkBox.BackColor = Color.Snow;
        }

        private void OnCheckBoxMouseLeave(CheckBox checkBox)
        {
            RoomNodeControl_MouseLeave(this, null);
            checkBox.BackColor = Color.Transparent;
        }

        private void RoomNodeControl_MouseEnter(object sender, EventArgs e)
        {
            this.BackColor = Color.Gold;                        
        }

        private void RoomNodeControl_MouseLeave(object sender, EventArgs e)
        {
            this.BackColor = Color.Transparent;                        
        }
                        
        private void lockedCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            lockedCheckbox.BackgroundImage = lockedCheckbox.Checked ?
                global::AGS.Editor.Properties.Resources.lockResource :
                global::AGS.Editor.Properties.Resources.lock_open;
        }

        private void visibleCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            visibleCheckbox.BackgroundImage = visibleCheckbox.Checked ?
                global::AGS.Editor.Properties.Resources.eye :
                global::AGS.Editor.Properties.Resources.eye_closed;
        }        
    }
}
