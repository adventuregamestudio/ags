namespace AGS.Editor
{
    partial class GeneralSettings
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
            this.gameSettings = new System.Windows.Forms.PropertyGrid();
            this.SuspendLayout();
            // 
            // gameSettings
            // 
            this.gameSettings.Dock = System.Windows.Forms.DockStyle.Fill;
            this.gameSettings.Location = new System.Drawing.Point(0, 0);
            this.gameSettings.Name = "gameSettings";
            this.gameSettings.Size = new System.Drawing.Size(501, 494);
            this.gameSettings.TabIndex = 20;
            this.gameSettings.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.gameSettings_PropertyValueChanged);
            // 
            // GeneralSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.gameSettings);
            this.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.Name = "GeneralSettings";
            this.Size = new System.Drawing.Size(501, 494);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PropertyGrid gameSettings;
    }
}
