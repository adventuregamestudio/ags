namespace AGS.Editor
{
    partial class RoomMessagesEditor
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
            this.lstList = new System.Windows.Forms.ListView();
            this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
            this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
            this.txtMessage = new System.Windows.Forms.TextBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.chkAutoRemove = new System.Windows.Forms.CheckBox();
            this.chkShowNextMessage = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.grpMessageDetails = new System.Windows.Forms.GroupBox();
            this.cmbDisplayAs = new System.Windows.Forms.ComboBox();
            this.grpMessageDetails.SuspendLayout();
            this.SuspendLayout();
            // 
            // lstList
            // 
            this.lstList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this.lstList.FullRowSelect = true;
            this.lstList.GridLines = true;
            this.lstList.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.lstList.HideSelection = false;
            this.lstList.Location = new System.Drawing.Point(12, 11);
            this.lstList.MultiSelect = false;
            this.lstList.Name = "lstList";
            this.lstList.Size = new System.Drawing.Size(511, 136);
            this.lstList.TabIndex = 0;
            this.lstList.UseCompatibleStateImageBehavior = false;
            this.lstList.View = System.Windows.Forms.View.Details;
            this.lstList.SelectedIndexChanged += new System.EventHandler(this.lstList_SelectedIndexChanged);
            this.lstList.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lstList_MouseUp);
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Number";
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Text";
            this.columnHeader2.Width = 410;
            // 
            // txtMessage
            // 
            this.txtMessage.Location = new System.Drawing.Point(8, 20);
            this.txtMessage.Multiline = true;
            this.txtMessage.Name = "txtMessage";
            this.txtMessage.Size = new System.Drawing.Size(494, 102);
            this.txtMessage.TabIndex = 1;
            this.txtMessage.TextChanged += new System.EventHandler(this.txtMessage_TextChanged);
            // 
            // btnClose
            // 
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(12, 367);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(106, 24);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "&Close";
            this.btnClose.UseVisualStyleBackColor = true;
            // 
            // chkAutoRemove
            // 
            this.chkAutoRemove.AutoSize = true;
            this.chkAutoRemove.Location = new System.Drawing.Point(8, 128);
            this.chkAutoRemove.Name = "chkAutoRemove";
            this.chkAutoRemove.Size = new System.Drawing.Size(372, 17);
            this.chkAutoRemove.TabIndex = 3;
            this.chkAutoRemove.Text = "Message will be removed automatically without the player having to click";
            this.chkAutoRemove.UseVisualStyleBackColor = true;
            this.chkAutoRemove.CheckedChanged += new System.EventHandler(this.chkAutoRemove_CheckedChanged);
            // 
            // chkShowNextMessage
            // 
            this.chkShowNextMessage.AutoSize = true;
            this.chkShowNextMessage.Location = new System.Drawing.Point(8, 151);
            this.chkShowNextMessage.Name = "chkShowNextMessage";
            this.chkShowNextMessage.Size = new System.Drawing.Size(257, 17);
            this.chkShowNextMessage.TabIndex = 4;
            this.chkShowNextMessage.Text = "Display the next message straight after this one";
            this.chkShowNextMessage.UseVisualStyleBackColor = true;
            this.chkShowNextMessage.CheckedChanged += new System.EventHandler(this.chkShowNextMessage_CheckedChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 180);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(123, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Display the message as:";
            // 
            // grpMessageDetails
            // 
            this.grpMessageDetails.Controls.Add(this.cmbDisplayAs);
            this.grpMessageDetails.Controls.Add(this.label1);
            this.grpMessageDetails.Controls.Add(this.chkShowNextMessage);
            this.grpMessageDetails.Controls.Add(this.chkAutoRemove);
            this.grpMessageDetails.Controls.Add(this.txtMessage);
            this.grpMessageDetails.Location = new System.Drawing.Point(12, 153);
            this.grpMessageDetails.Name = "grpMessageDetails";
            this.grpMessageDetails.Size = new System.Drawing.Size(515, 208);
            this.grpMessageDetails.TabIndex = 7;
            this.grpMessageDetails.TabStop = false;
            this.grpMessageDetails.Text = "Selected message details";
            // 
            // cmbDisplayAs
            // 
            this.cmbDisplayAs.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbDisplayAs.FormattingEnabled = true;
            this.cmbDisplayAs.Location = new System.Drawing.Point(134, 177);
            this.cmbDisplayAs.Name = "cmbDisplayAs";
            this.cmbDisplayAs.Size = new System.Drawing.Size(156, 21);
            this.cmbDisplayAs.TabIndex = 7;
            this.cmbDisplayAs.SelectedIndexChanged += new System.EventHandler(this.cmbDisplayAs_SelectedIndexChanged);
            // 
            // RoomMessagesEditor
            // 
            this.AcceptButton = this.btnClose;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnClose;
            this.ClientSize = new System.Drawing.Size(539, 403);
            this.Controls.Add(this.grpMessageDetails);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.lstList);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "RoomMessagesEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Room Messages";
            this.grpMessageDetails.ResumeLayout(false);
            this.grpMessageDetails.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView lstList;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.TextBox txtMessage;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.CheckBox chkAutoRemove;
        private System.Windows.Forms.CheckBox chkShowNextMessage;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox grpMessageDetails;
        private System.Windows.Forms.ComboBox cmbDisplayAs;
    }
}