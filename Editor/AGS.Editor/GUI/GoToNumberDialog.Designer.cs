namespace AGS.Editor
{
    partial class GoToNumberDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.lblNodeNumberRange = new System.Windows.Forms.Label();
            this.upDownNumber = new System.Windows.Forms.NumericUpDown();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.lstNodes = new System.Windows.Forms.ListBox();
            ((System.ComponentModel.ISupportInitialize)(this.upDownNumber)).BeginInit();
            this.SuspendLayout();
            // 
            // lblNodeNumberRange
            // 
            this.lblNodeNumberRange.AutoSize = true;
            this.lblNodeNumberRange.Location = new System.Drawing.Point(15, 11);
            this.lblNodeNumberRange.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.lblNodeNumberRange.Name = "lblNodeNumberRange";
            this.lblNodeNumberRange.Size = new System.Drawing.Size(174, 16);
            this.lblNodeNumberRange.TabIndex = 0;
            this.lblNodeNumberRange.Text = "Select or type room number:";
            // 
            // upDownNumber
            // 
            this.upDownNumber.Location = new System.Drawing.Point(19, 42);
            this.upDownNumber.Margin = new System.Windows.Forms.Padding(4);
            this.upDownNumber.Name = "upDownNumber";
            this.upDownNumber.Size = new System.Drawing.Size(249, 22);
            this.upDownNumber.TabIndex = 1;
            this.upDownNumber.ValueChanged += new System.EventHandler(this.upDownNumber_ValueChanged);
            this.upDownNumber.KeyUp += new System.Windows.Forms.KeyEventHandler(this.upDownNumber_KeyUp);
            // 
            // btnOk
            // 
            this.btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOk.Location = new System.Drawing.Point(74, 183);
            this.btnOk.Margin = new System.Windows.Forms.Padding(4);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(94, 29);
            this.btnOk.TabIndex = 2;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(176, 183);
            this.btnCancel.Margin = new System.Windows.Forms.Padding(4);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(94, 29);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // lstNodes
            // 
            this.lstNodes.FormattingEnabled = true;
            this.lstNodes.ItemHeight = 16;
            this.lstNodes.Location = new System.Drawing.Point(18, 72);
            this.lstNodes.Name = "lstNodes";
            this.lstNodes.Size = new System.Drawing.Size(250, 100);
            this.lstNodes.TabIndex = 4;
            this.lstNodes.SelectedIndexChanged += new System.EventHandler(this.lstNodes_SelectedIndexChanged);
            this.lstNodes.Format += new System.Windows.Forms.ListControlConvertEventHandler(this.lstNodes_Format);
            this.lstNodes.SelectedValueChanged += new System.EventHandler(this.lstNodes_SelectedValueChanged);
            this.lstNodes.DoubleClick += new System.EventHandler(this.lstNodes_DoubleClick);
            // 
            // GoToNumberDialog
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(120F, 120F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(282, 225);
            this.Controls.Add(this.lstNodes);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.upDownNumber);
            this.Controls.Add(this.lblNodeNumberRange);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "GoToNumberDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Go To Number";
            this.TopMost = true;
            ((System.ComponentModel.ISupportInitialize)(this.upDownNumber)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblNodeNumberRange;
        private System.Windows.Forms.NumericUpDown upDownNumber;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ListBox lstNodes;
    }
}