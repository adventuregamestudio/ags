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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PropertiesPanel));
            this.propertiesGrid = new System.Windows.Forms.PropertyGrid();
            this.propertyObjectCombo = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // propertiesGrid
            // 
            this.propertiesGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesGrid.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.propertiesGrid.Location = new System.Drawing.Point(1, 44);
            this.propertiesGrid.Name = "propertiesGrid";
            this.propertiesGrid.Size = new System.Drawing.Size(290, 215);
            this.propertiesGrid.TabIndex = 12;
            // 
            // propertyObjectCombo
            // 
            this.propertyObjectCombo.Dock = System.Windows.Forms.DockStyle.Top;
            this.propertyObjectCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.propertyObjectCombo.FormattingEnabled = true;
            this.propertyObjectCombo.Location = new System.Drawing.Point(1, 20);
            this.propertyObjectCombo.Name = "propertyObjectCombo";
            this.propertyObjectCombo.Size = new System.Drawing.Size(290, 24);
            this.propertyObjectCombo.TabIndex = 11;
            // 
            // PropertiesPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(292, 260);
            this.Controls.Add(this.propertiesGrid);
            this.Controls.Add(this.propertyObjectCombo);
            this.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "PropertiesPanel";
            this.Padding = new System.Windows.Forms.Padding(1, 20, 1, 1);
            this.VisibleChanged += new System.EventHandler(this.PropertiesPanel_VisibleChanged);
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.PropertyGrid propertiesGrid;
        internal System.Windows.Forms.ComboBox propertyObjectCombo;
    }
}
