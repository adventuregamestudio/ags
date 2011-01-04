namespace AGS.Editor
{
	partial class HelpPopup
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(HelpPopup));
			this.panel1 = new System.Windows.Forms.Panel();
			this.btnStopBuggingMe = new System.Windows.Forms.Button();
			this.btnOK = new System.Windows.Forms.Button();
			this.lblHelpText = new System.Windows.Forms.Label();
			this.panel1.SuspendLayout();
			this.SuspendLayout();
			// 
			// panel1
			// 
			this.panel1.BackColor = System.Drawing.Color.Wheat;
			this.panel1.Controls.Add(this.btnStopBuggingMe);
			this.panel1.Controls.Add(this.btnOK);
			this.panel1.Controls.Add(this.lblHelpText);
			this.panel1.Location = new System.Drawing.Point(172, 11);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(321, 160);
			this.panel1.TabIndex = 3;
			// 
			// btnStopBuggingMe
			// 
			this.btnStopBuggingMe.Location = new System.Drawing.Point(140, 127);
			this.btnStopBuggingMe.Name = "btnStopBuggingMe";
			this.btnStopBuggingMe.Size = new System.Drawing.Size(123, 29);
			this.btnStopBuggingMe.TabIndex = 5;
			this.btnStopBuggingMe.Text = "Stop bugging me";
			this.btnStopBuggingMe.UseVisualStyleBackColor = true;
			this.btnStopBuggingMe.Click += new System.EventHandler(this.btnStopBuggingMe_Click);
			// 
			// btnOK
			// 
			this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnOK.Location = new System.Drawing.Point(9, 127);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(113, 29);
			this.btnOK.TabIndex = 4;
			this.btnOK.Text = "OK!";
			this.btnOK.UseVisualStyleBackColor = true;
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// lblHelpText
			// 
			this.lblHelpText.AutoSize = true;
			this.lblHelpText.BackColor = System.Drawing.Color.Transparent;
			this.lblHelpText.Location = new System.Drawing.Point(5, 5);
			this.lblHelpText.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
			this.lblHelpText.MaximumSize = new System.Drawing.Size(305, 115);
			this.lblHelpText.Name = "lblHelpText";
			this.lblHelpText.Size = new System.Drawing.Size(304, 115);
			this.lblHelpText.TabIndex = 3;
			this.lblHelpText.Text = resources.GetString("lblHelpText.Text");
			// 
			// HelpPopup
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
			this.CancelButton = this.btnOK;
			this.ClientSize = new System.Drawing.Size(500, 181);
			this.Controls.Add(this.panel1);
			this.Font = new System.Drawing.Font("Comic Sans MS", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
			this.Margin = new System.Windows.Forms.Padding(4);
			this.Name = "HelpPopup";
			this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
			this.Text = "AGS Help";
			this.TransparencyKey = System.Drawing.Color.White;
			this.panel1.ResumeLayout(false);
			this.panel1.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Button btnStopBuggingMe;
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Label lblHelpText;

	}
}