namespace AGS.Editor
{
    partial class MultilineStringEditorDialog
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
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.tableLayoutDialog = new System.Windows.Forms.TableLayoutPanel();
            this.tableLayoutBottom = new System.Windows.Forms.TableLayoutPanel();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.labelStatus = new System.Windows.Forms.Label();
            this.tableLayoutDialog.SuspendLayout();
            this.tableLayoutBottom.SuspendLayout();
            this.SuspendLayout();
            // 
            // textBox1
            // 
            this.textBox1.AcceptsReturn = true;
            this.textBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textBox1.Location = new System.Drawing.Point(3, 3);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.textBox1.Size = new System.Drawing.Size(580, 239);
            this.textBox1.TabIndex = 0;
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox1_KeyDown);
            // 
            // tableLayoutDialog
            // 
            this.tableLayoutDialog.AutoSize = true;
            this.tableLayoutDialog.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutDialog.ColumnCount = 1;
            this.tableLayoutDialog.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutDialog.Controls.Add(this.textBox1, 0, 0);
            this.tableLayoutDialog.Controls.Add(this.tableLayoutBottom, 0, 1);
            this.tableLayoutDialog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutDialog.GrowStyle = System.Windows.Forms.TableLayoutPanelGrowStyle.FixedSize;
            this.tableLayoutDialog.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutDialog.MinimumSize = new System.Drawing.Size(300, 200);
            this.tableLayoutDialog.Name = "tableLayoutDialog";
            this.tableLayoutDialog.RowCount = 2;
            this.tableLayoutDialog.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutDialog.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 55F));
            this.tableLayoutDialog.Size = new System.Drawing.Size(586, 300);
            this.tableLayoutDialog.TabIndex = 1;
            // 
            // tableLayoutBottom
            // 
            this.tableLayoutBottom.ColumnCount = 3;
            this.tableLayoutBottom.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutBottom.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 137F));
            this.tableLayoutBottom.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 137F));
            this.tableLayoutBottom.Controls.Add(this.btnOk, 1, 0);
            this.tableLayoutBottom.Controls.Add(this.btnCancel, 2, 0);
            this.tableLayoutBottom.Controls.Add(this.labelStatus, 0, 0);
            this.tableLayoutBottom.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutBottom.GrowStyle = System.Windows.Forms.TableLayoutPanelGrowStyle.FixedSize;
            this.tableLayoutBottom.Location = new System.Drawing.Point(3, 248);
            this.tableLayoutBottom.MinimumSize = new System.Drawing.Size(200, 49);
            this.tableLayoutBottom.Name = "tableLayoutBottom";
            this.tableLayoutBottom.RowCount = 1;
            this.tableLayoutBottom.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutBottom.Size = new System.Drawing.Size(580, 49);
            this.tableLayoutBottom.TabIndex = 1;
            // 
            // btnOk
            // 
            this.btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOk.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnOk.Location = new System.Drawing.Point(314, 8);
            this.btnOk.Margin = new System.Windows.Forms.Padding(8);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(121, 33);
            this.btnOk.TabIndex = 0;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.btnCancel.Location = new System.Drawing.Point(451, 8);
            this.btnCancel.Margin = new System.Windows.Forms.Padding(8);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(121, 33);
            this.btnCancel.TabIndex = 1;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // labelStatus
            // 
            this.labelStatus.AutoSize = true;
            this.labelStatus.Dock = System.Windows.Forms.DockStyle.Fill;
            this.labelStatus.Location = new System.Drawing.Point(3, 0);
            this.labelStatus.Name = "labelStatus";
            this.labelStatus.Size = new System.Drawing.Size(300, 49);
            this.labelStatus.TabIndex = 2;
            this.labelStatus.Text = "label1";
            this.labelStatus.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // MultilineStringEditorDialog
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(586, 300);
            this.Controls.Add(this.tableLayoutDialog);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(300, 200);
            this.Name = "MultilineStringEditorDialog";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Edit text...";
            this.Load += new System.EventHandler(this.MultilineStringEditorDialog_Load);
            this.tableLayoutDialog.ResumeLayout(false);
            this.tableLayoutDialog.PerformLayout();
            this.tableLayoutBottom.ResumeLayout(false);
            this.tableLayoutBottom.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutDialog;
        private System.Windows.Forms.TableLayoutPanel tableLayoutBottom;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label labelStatus;
    }
}