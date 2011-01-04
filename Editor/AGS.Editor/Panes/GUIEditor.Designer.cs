namespace AGS.Editor
{
    partial class GUIEditor
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
            this.bgPanel = new AGS.Editor.BufferedPanel();
            this.SuspendLayout();
            // 
            // bgPanel
            // 
            this.bgPanel.BackColor = System.Drawing.Color.Transparent;
            this.bgPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.bgPanel.Location = new System.Drawing.Point(0, 0);
            this.bgPanel.Name = "bgPanel";
            this.bgPanel.Size = new System.Drawing.Size(702, 459);
            this.bgPanel.TabIndex = 1;
            this.bgPanel.MouseLeave += new System.EventHandler(this.bgPanel_MouseLeave);
            this.bgPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseDown);
            this.bgPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseMove);
            this.bgPanel.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseDoubleClick);
            this.bgPanel.MouseEnter += new System.EventHandler(this.bgPanel_MouseEnter);
            this.bgPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.bgPanel_Paint);
            this.bgPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseUp);
            // 
            // GUIEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.bgPanel);
            this.Name = "GUIEditor";
            this.Size = new System.Drawing.Size(702, 459);
            this.ResumeLayout(false);

        }

        #endregion

        private BufferedPanel bgPanel;
    }
}
