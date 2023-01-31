namespace AGS.Editor
{
    partial class StartNewGameWizardPage
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.tbDescription = new System.Windows.Forms.TextBox();
            this.lstTemplates = new System.Windows.Forms.ListView();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.tbDescription);
            this.groupBox1.Controls.Add(this.lstTemplates);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 7);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(520, 320);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Select Template";
            // 
            // tbDescription
            // 
            this.tbDescription.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbDescription.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.tbDescription.Location = new System.Drawing.Point(16, 215);
            this.tbDescription.Multiline = true;
            this.tbDescription.Name = "tbDescription";
            this.tbDescription.ReadOnly = true;
            this.tbDescription.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tbDescription.Size = new System.Drawing.Size(489, 99);
            this.tbDescription.TabIndex = 2;
            // 
            // lstTemplates
            // 
            this.lstTemplates.HideSelection = false;
            this.lstTemplates.Location = new System.Drawing.Point(14, 41);
            this.lstTemplates.MultiSelect = false;
            this.lstTemplates.Name = "lstTemplates";
            this.lstTemplates.Size = new System.Drawing.Size(491, 167);
            this.lstTemplates.TabIndex = 1;
            this.lstTemplates.UseCompatibleStateImageBehavior = false;
            this.lstTemplates.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.lstTemplates_ItemSelectionChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 19);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(425, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Your new game is created based on a template. Please select the one you\'d like to" +
    " use:";
            // 
            // StartNewGameWizardPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MinimumSize = new System.Drawing.Size(520, 425);
            this.Name = "StartNewGameWizardPage";
            this.Size = new System.Drawing.Size(1504, 580);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.ListView lstTemplates;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbDescription;
    }
}
