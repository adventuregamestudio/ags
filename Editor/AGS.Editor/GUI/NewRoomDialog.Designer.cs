namespace AGS.Editor
{
	partial class NewRoomDialog
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
            this.lstRoomTemplates = new System.Windows.Forms.ListView();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.chkNonStateSaving = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.udRoomNumber = new System.Windows.Forms.NumericUpDown();
            this.label3 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.udRoomNumber)).BeginInit();
            this.SuspendLayout();
            // 
            // lstRoomTemplates
            // 
            this.lstRoomTemplates.HideSelection = false;
            this.lstRoomTemplates.Location = new System.Drawing.Point(18, 25);
            this.lstRoomTemplates.MultiSelect = false;
            this.lstRoomTemplates.Name = "lstRoomTemplates";
            this.lstRoomTemplates.Size = new System.Drawing.Size(583, 262);
            this.lstRoomTemplates.TabIndex = 0;
            this.lstRoomTemplates.UseCompatibleStateImageBehavior = false;
            // 
            // btnOk
            // 
            this.btnOk.Location = new System.Drawing.Point(20, 361);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(109, 29);
            this.btnOk.TabIndex = 1;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(145, 361);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(117, 28);
            this.btnCancel.TabIndex = 2;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(15, 7);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(255, 13);
            this.label1.TabIndex = 3;
            this.label1.Text = "Please choose the type of room you wish to create:";
            // 
            // chkNonStateSaving
            // 
            this.chkNonStateSaving.AutoSize = true;
            this.chkNonStateSaving.Location = new System.Drawing.Point(18, 293);
            this.chkNonStateSaving.Name = "chkNonStateSaving";
            this.chkNonStateSaving.Size = new System.Drawing.Size(565, 17);
            this.chkNonStateSaving.TabIndex = 4;
            this.chkNonStateSaving.Text = "Do not save state in this room (when the player leaves the room, everything will " +
                "be reset back to its initial state)";
            this.chkNonStateSaving.UseVisualStyleBackColor = true;
            this.chkNonStateSaving.CheckedChanged += new System.EventHandler(this.chkNonStateSaving_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(17, 318);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "Room number:";
            // 
            // udRoomNumber
            // 
            this.udRoomNumber.Location = new System.Drawing.Point(100, 315);
            this.udRoomNumber.Maximum = new decimal(new int[] {
            999,
            0,
            0,
            0});
            this.udRoomNumber.Name = "udRoomNumber";
            this.udRoomNumber.Size = new System.Drawing.Size(67, 21);
            this.udRoomNumber.TabIndex = 6;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(17, 339);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(366, 13);
            this.label3.TabIndex = 7;
            this.label3.Text = "Room numbers below 300 save the room state; numbers above 300 do not";
            // 
            // NewRoomDialog
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(625, 402);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.udRoomNumber);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.chkNonStateSaving);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.lstRoomTemplates);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "NewRoomDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Create New Room";
            ((System.ComponentModel.ISupportInitialize)(this.udRoomNumber)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.ListView lstRoomTemplates;
		private System.Windows.Forms.Button btnOk;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.CheckBox chkNonStateSaving;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.NumericUpDown udRoomNumber;
		private System.Windows.Forms.Label label3;
	}
}