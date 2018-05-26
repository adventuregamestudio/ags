namespace AGS.Editor.Panes.Room
{
    partial class RoomNodeControl
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
            this.label = new System.Windows.Forms.Label();
            this.visibleCheckbox = new System.Windows.Forms.CheckBox();
            this.lockedCheckbox = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // label
            // 
            this.label.AutoSize = true;
            this.label.Location = new System.Drawing.Point(54, 9);
            this.label.Name = "label";
            this.label.Size = new System.Drawing.Size(27, 13);
            this.label.TabIndex = 0;
            this.label.Text = "Item";
            // 
            // visibleCheckbox
            // 
            this.visibleCheckbox.Appearance = System.Windows.Forms.Appearance.Button;
            this.visibleCheckbox.AutoSize = true;
            this.visibleCheckbox.BackgroundImage = global::AGS.Editor.Properties.Resources.eye;
            this.visibleCheckbox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.visibleCheckbox.Location = new System.Drawing.Point(27, 3);
            this.visibleCheckbox.MinimumSize = new System.Drawing.Size(23, 24);
            this.visibleCheckbox.Name = "visibleCheckbox";
            this.visibleCheckbox.Size = new System.Drawing.Size(23, 24);
            this.visibleCheckbox.TabIndex = 1;
            this.visibleCheckbox.UseVisualStyleBackColor = true;
            // 
            // lockedCheckbox
            // 
            this.lockedCheckbox.Appearance = System.Windows.Forms.Appearance.Button;
            this.lockedCheckbox.AutoSize = true;
            this.lockedCheckbox.BackgroundImage = global::AGS.Editor.Properties.Resources.lock_open;
            this.lockedCheckbox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.lockedCheckbox.Location = new System.Drawing.Point(3, 3);
            this.lockedCheckbox.MinimumSize = new System.Drawing.Size(23, 24);
            this.lockedCheckbox.Name = "lockedCheckbox";
            this.lockedCheckbox.Size = new System.Drawing.Size(23, 24);
            this.lockedCheckbox.TabIndex = 2;
            this.lockedCheckbox.UseVisualStyleBackColor = true;
            // 
            // RoomNodeControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.lockedCheckbox);
            this.Controls.Add(this.visibleCheckbox);
            this.Controls.Add(this.label);
            this.Name = "RoomNodeControl";
            this.Size = new System.Drawing.Size(211, 30);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label;
        private System.Windows.Forms.CheckBox visibleCheckbox;
        private System.Windows.Forms.CheckBox lockedCheckbox;
    }
}
