namespace AGS.Editor
{
    partial class PropertiesPanel
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PropertiesPanel));
            this.propertiesGrid = new System.Windows.Forms.PropertyGrid();
            this.propertyObjectCombo = new System.Windows.Forms.ComboBox();
            this.ctxmenuPropertyGrid = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.copyPropertyNameToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.copyPropertyValueToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ctxmenuPropertyGrid.SuspendLayout();
            this.SuspendLayout();
            // 
            // propertiesGrid
            // 
            this.propertiesGrid.ContextMenuStrip = this.ctxmenuPropertyGrid;
            this.propertiesGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.propertiesGrid.Location = new System.Drawing.Point(0, 21);
            this.propertiesGrid.Name = "propertiesGrid";
            this.propertiesGrid.Size = new System.Drawing.Size(292, 239);
            this.propertiesGrid.TabIndex = 12;
            // 
            // propertyObjectCombo
            // 
            this.propertyObjectCombo.Dock = System.Windows.Forms.DockStyle.Top;
            this.propertyObjectCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.propertyObjectCombo.FormattingEnabled = true;
            this.propertyObjectCombo.Location = new System.Drawing.Point(0, 0);
            this.propertyObjectCombo.Name = "propertyObjectCombo";
            this.propertyObjectCombo.Size = new System.Drawing.Size(292, 21);
            this.propertyObjectCombo.TabIndex = 11;
            // 
            // ctxmenuPropertyGrid
            // 
            this.ctxmenuPropertyGrid.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyPropertyNameToolStripMenuItem,
            this.copyPropertyValueToolStripMenuItem});
            this.ctxmenuPropertyGrid.Name = "ctxmenuPropertyGrid";
            this.ctxmenuPropertyGrid.Size = new System.Drawing.Size(175, 48);
            // 
            // copyPropertyNameToolStripMenuItem
            // 
            this.copyPropertyNameToolStripMenuItem.Name = "copyPropertyNameToolStripMenuItem";
            this.copyPropertyNameToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.copyPropertyNameToolStripMenuItem.Text = "Copy Property Name";
            this.copyPropertyNameToolStripMenuItem.Click += new System.EventHandler(this.copyPropertyNameToolStripMenuItem_Click);
            // 
            // copyPropertyValueToolStripMenuItem
            // 
            this.copyPropertyValueToolStripMenuItem.Name = "copyPropertyValueToolStripMenuItem";
            this.copyPropertyValueToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.copyPropertyValueToolStripMenuItem.Text = "Copy Property Value";
            this.copyPropertyValueToolStripMenuItem.Click += new System.EventHandler(this.copyPropertyValueToolStripMenuItem_Click);
            // 
            // PropertiesPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(292, 260);
            this.Controls.Add(this.propertiesGrid);
            this.Controls.Add(this.propertyObjectCombo);
            this.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "PropertiesPanel";
            this.Load += new System.EventHandler(this.PropertiesPanel_Load);
            this.VisibleChanged += new System.EventHandler(this.PropertiesPanel_VisibleChanged);
            this.ctxmenuPropertyGrid.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PropertyGrid propertiesGrid;
        internal System.Windows.Forms.ComboBox propertyObjectCombo;
        private System.Windows.Forms.ContextMenuStrip ctxmenuPropertyGrid;
        private System.Windows.Forms.ToolStripMenuItem copyPropertyNameToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem copyPropertyValueToolStripMenuItem;
    }
}
