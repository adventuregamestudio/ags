
namespace AGS.Editor
{
    partial class ZoomTrackbar
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
            this.sldZoomLevel = new System.Windows.Forms.TrackBar();
            this.labelName = new System.Windows.Forms.Label();
            this.textBoxZoomValue = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).BeginInit();
            this.SuspendLayout();
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.sldZoomLevel.Location = new System.Drawing.Point(34, 0);
            this.sldZoomLevel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(88, 42);
            this.sldZoomLevel.TabIndex = 0;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.sldZoomLevel_ValueChanged);
            // 
            // labelName
            // 
            this.labelName.AutoSize = true;
            this.labelName.Location = new System.Drawing.Point(2, 8);
            this.labelName.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.labelName.Name = "labelName";
            this.labelName.Size = new System.Drawing.Size(37, 13);
            this.labelName.TabIndex = 3;
            this.labelName.Text = "Zoom:";
            // 
            // textBoxZoomValue
            // 
            this.textBoxZoomValue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxZoomValue.Location = new System.Drawing.Point(127, 4);
            this.textBoxZoomValue.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.textBoxZoomValue.MaxLength = 6;
            this.textBoxZoomValue.Name = "textBoxZoomValue";
            this.textBoxZoomValue.Size = new System.Drawing.Size(36, 20);
            this.textBoxZoomValue.TabIndex = 4;
            this.textBoxZoomValue.Text = "100%";
            this.textBoxZoomValue.WordWrap = false;
            this.textBoxZoomValue.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxZoomValue_KeyPress);
            this.textBoxZoomValue.Validating += new System.ComponentModel.CancelEventHandler(this.textBoxZoomValue_Validating);
            this.textBoxZoomValue.Validated += new System.EventHandler(this.textBoxZoomValue_Validated);
            // 
            // ZoomTrackbar
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.textBoxZoomValue);
            this.Controls.Add(this.sldZoomLevel);
            this.Controls.Add(this.labelName);
            this.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.Name = "ZoomTrackbar";
            this.Size = new System.Drawing.Size(166, 25);
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TrackBar sldZoomLevel;
        private System.Windows.Forms.Label labelName;
        private System.Windows.Forms.TextBox textBoxZoomValue;
    }
}
