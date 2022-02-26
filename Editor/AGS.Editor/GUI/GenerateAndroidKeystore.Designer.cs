
namespace AGS.Editor
{
    partial class GenerateAndroidKeystore
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GenerateAndroidKeystore));
            this.panel1 = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.buttonGenerate = new System.Windows.Forms.Button();
            this.buttonOK = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.labelExplanation = new System.Windows.Forms.Label();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.textboxKeystorePassword = new System.Windows.Forms.TextBox();
            this.textboxKeystorePath = new System.Windows.Forms.TextBox();
            this.labelKeystore_Path = new System.Windows.Forms.Label();
            this.labelKeystorePass = new System.Windows.Forms.Label();
            this.buttonBrowse = new System.Windows.Forms.Button();
            this.groupBoxKey = new System.Windows.Forms.GroupBox();
            this.groupBoxCertificate = new System.Windows.Forms.GroupBox();
            this.tableLayoutPanel3 = new System.Windows.Forms.TableLayoutPanel();
            this.textboxKeystoreCertCN = new System.Windows.Forms.TextBox();
            this.label17 = new System.Windows.Forms.Label();
            this.textboxKeystoreCertC = new System.Windows.Forms.TextBox();
            this.label22 = new System.Windows.Forms.Label();
            this.label18 = new System.Windows.Forms.Label();
            this.textboxKeystoreCertOU = new System.Windows.Forms.TextBox();
            this.textboxKeystoreCertST = new System.Windows.Forms.TextBox();
            this.label21 = new System.Windows.Forms.Label();
            this.label19 = new System.Windows.Forms.Label();
            this.textboxKeystoreCertO = new System.Windows.Forms.TextBox();
            this.textboxKeystoreCertL = new System.Windows.Forms.TextBox();
            this.label20 = new System.Windows.Forms.Label();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.textboxKeystoreKeyPassword = new System.Windows.Forms.TextBox();
            this.labelKeystoreKeyAlias = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.textboxKeystoreKeyAlias = new System.Windows.Forms.TextBox();
            this.numericValidityYears = new System.Windows.Forms.NumericUpDown();
            this.labelKeystoreKeyPass = new System.Windows.Forms.Label();
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.panel1.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.groupBoxKey.SuspendLayout();
            this.groupBoxCertificate.SuspendLayout();
            this.tableLayoutPanel3.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numericValidityYears)).BeginInit();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.flowLayoutPanel2);
            this.panel1.Controls.Add(this.labelExplanation);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 425);
            this.panel1.MinimumSize = new System.Drawing.Size(0, 60);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(818, 89);
            this.panel1.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.label1.Location = new System.Drawing.Point(0, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(634, 17);
            this.label1.TabIndex = 0;
            this.label1.Text = "You won\'t be able to change the signing configuration after you submit to Play St" +
    "ore or other store. ";
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.AutoSize = true;
            this.flowLayoutPanel2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel2.Controls.Add(this.buttonGenerate);
            this.flowLayoutPanel2.Controls.Add(this.buttonOK);
            this.flowLayoutPanel2.Controls.Add(this.buttonCancel);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(0, 56);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(818, 33);
            this.flowLayoutPanel2.TabIndex = 1;
            // 
            // buttonGenerate
            // 
            this.buttonGenerate.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.buttonGenerate.AutoSize = true;
            this.buttonGenerate.Location = new System.Drawing.Point(3, 3);
            this.buttonGenerate.Name = "buttonGenerate";
            this.buttonGenerate.Size = new System.Drawing.Size(138, 27);
            this.buttonGenerate.TabIndex = 0;
            this.buttonGenerate.Text = "Generate Keystore";
            this.buttonGenerate.UseVisualStyleBackColor = true;
            this.buttonGenerate.Click += new System.EventHandler(this.buttonGenerate_Click);
            // 
            // buttonOK
            // 
            this.buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.buttonOK.AutoSize = true;
            this.buttonOK.Location = new System.Drawing.Point(147, 3);
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.Size = new System.Drawing.Size(75, 27);
            this.buttonOK.TabIndex = 1;
            this.buttonOK.Text = "OK";
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // buttonCancel
            // 
            this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.buttonCancel.AutoSize = true;
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(228, 3);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(75, 27);
            this.buttonCancel.TabIndex = 2;
            this.buttonCancel.Text = "Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            // 
            // labelExplanation
            // 
            this.labelExplanation.AutoSize = true;
            this.labelExplanation.Dock = System.Windows.Forms.DockStyle.Top;
            this.labelExplanation.Location = new System.Drawing.Point(0, 0);
            this.labelExplanation.Name = "labelExplanation";
            this.labelExplanation.Size = new System.Drawing.Size(596, 17);
            this.labelExplanation.TabIndex = 0;
            this.labelExplanation.Text = "WARNING: Please backup all information!  No certificate information is stored in " +
    "your project!!!";
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.panel2);
            this.flowLayoutPanel1.Controls.Add(this.richTextBox1);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(818, 425);
            this.flowLayoutPanel1.TabIndex = 1;
            // 
            // panel2
            // 
            this.panel2.AutoSize = true;
            this.panel2.Controls.Add(this.tableLayoutPanel2);
            this.panel2.Controls.Add(this.groupBoxKey);
            this.panel2.Location = new System.Drawing.Point(3, 3);
            this.panel2.MinimumSize = new System.Drawing.Size(464, 400);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(464, 404);
            this.panel2.TabIndex = 5;
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.ColumnCount = 3;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 38.09524F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 61.90476F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 47F));
            this.tableLayoutPanel2.Controls.Add(this.textboxKeystorePassword, 1, 1);
            this.tableLayoutPanel2.Controls.Add(this.textboxKeystorePath, 1, 0);
            this.tableLayoutPanel2.Controls.Add(this.labelKeystore_Path, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.labelKeystorePass, 0, 1);
            this.tableLayoutPanel2.Controls.Add(this.buttonBrowse, 2, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel2.MinimumSize = new System.Drawing.Size(464, 64);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 2;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(464, 64);
            this.tableLayoutPanel2.TabIndex = 3;
            // 
            // textboxKeystorePassword
            // 
            this.textboxKeystorePassword.Location = new System.Drawing.Point(161, 35);
            this.textboxKeystorePassword.MaxLength = 64;
            this.textboxKeystorePassword.MinimumSize = new System.Drawing.Size(240, 24);
            this.textboxKeystorePassword.Name = "textboxKeystorePassword";
            this.textboxKeystorePassword.Size = new System.Drawing.Size(240, 24);
            this.textboxKeystorePassword.TabIndex = 18;
            this.textboxKeystorePassword.TextChanged += new System.EventHandler(this.textboxKeystorePassword_TextChanged);
            this.textboxKeystorePassword.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystorePassword_Validating);
            // 
            // textboxKeystorePath
            // 
            this.textboxKeystorePath.Location = new System.Drawing.Point(161, 3);
            this.textboxKeystorePath.MaxLength = 280;
            this.textboxKeystorePath.MinimumSize = new System.Drawing.Size(240, 24);
            this.textboxKeystorePath.Name = "textboxKeystorePath";
            this.textboxKeystorePath.Size = new System.Drawing.Size(240, 24);
            this.textboxKeystorePath.TabIndex = 17;
            this.textboxKeystorePath.TextChanged += new System.EventHandler(this.textboxKeystorePath_TextChanged);
            this.textboxKeystorePath.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystorePath_Validating);
            // 
            // labelKeystore_Path
            // 
            this.labelKeystore_Path.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelKeystore_Path.AutoSize = true;
            this.labelKeystore_Path.Location = new System.Drawing.Point(51, 0);
            this.labelKeystore_Path.Name = "labelKeystore_Path";
            this.labelKeystore_Path.Size = new System.Drawing.Size(104, 17);
            this.labelKeystore_Path.TabIndex = 13;
            this.labelKeystore_Path.Text = "Key store path:";
            // 
            // labelKeystorePass
            // 
            this.labelKeystorePass.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelKeystorePass.AutoSize = true;
            this.labelKeystorePass.Location = new System.Drawing.Point(82, 32);
            this.labelKeystorePass.Name = "labelKeystorePass";
            this.labelKeystorePass.Size = new System.Drawing.Size(73, 17);
            this.labelKeystorePass.TabIndex = 14;
            this.labelKeystorePass.Text = "Password:";
            // 
            // buttonBrowse
            // 
            this.buttonBrowse.Location = new System.Drawing.Point(419, 3);
            this.buttonBrowse.Name = "buttonBrowse";
            this.buttonBrowse.Size = new System.Drawing.Size(39, 24);
            this.buttonBrowse.TabIndex = 19;
            this.buttonBrowse.Text = "...";
            this.buttonBrowse.UseVisualStyleBackColor = true;
            this.buttonBrowse.Click += new System.EventHandler(this.buttonBrowse_Click);
            // 
            // groupBoxKey
            // 
            this.groupBoxKey.AutoSize = true;
            this.groupBoxKey.Controls.Add(this.groupBoxCertificate);
            this.groupBoxKey.Controls.Add(this.tableLayoutPanel1);
            this.groupBoxKey.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.groupBoxKey.Location = new System.Drawing.Point(0, 64);
            this.groupBoxKey.MinimumSize = new System.Drawing.Size(436, 340);
            this.groupBoxKey.Name = "groupBoxKey";
            this.groupBoxKey.Size = new System.Drawing.Size(464, 340);
            this.groupBoxKey.TabIndex = 4;
            this.groupBoxKey.TabStop = false;
            this.groupBoxKey.Text = "Key";
            // 
            // groupBoxCertificate
            // 
            this.groupBoxCertificate.AutoSize = true;
            this.groupBoxCertificate.Controls.Add(this.tableLayoutPanel3);
            this.groupBoxCertificate.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBoxCertificate.Location = new System.Drawing.Point(3, 118);
            this.groupBoxCertificate.MinimumSize = new System.Drawing.Size(430, 200);
            this.groupBoxCertificate.Name = "groupBoxCertificate";
            this.groupBoxCertificate.Size = new System.Drawing.Size(458, 219);
            this.groupBoxCertificate.TabIndex = 1;
            this.groupBoxCertificate.TabStop = false;
            this.groupBoxCertificate.Text = "Certificate";
            // 
            // tableLayoutPanel3
            // 
            this.tableLayoutPanel3.ColumnCount = 2;
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 39.62264F));
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 60.37736F));
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertCN, 1, 0);
            this.tableLayoutPanel3.Controls.Add(this.label17, 0, 0);
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertC, 1, 5);
            this.tableLayoutPanel3.Controls.Add(this.label22, 0, 5);
            this.tableLayoutPanel3.Controls.Add(this.label18, 0, 1);
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertOU, 1, 1);
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertST, 1, 4);
            this.tableLayoutPanel3.Controls.Add(this.label21, 0, 4);
            this.tableLayoutPanel3.Controls.Add(this.label19, 0, 2);
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertO, 1, 2);
            this.tableLayoutPanel3.Controls.Add(this.textboxKeystoreCertL, 1, 3);
            this.tableLayoutPanel3.Controls.Add(this.label20, 0, 3);
            this.tableLayoutPanel3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel3.Location = new System.Drawing.Point(3, 18);
            this.tableLayoutPanel3.Name = "tableLayoutPanel3";
            this.tableLayoutPanel3.RowCount = 6;
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 16.66667F));
            this.tableLayoutPanel3.Size = new System.Drawing.Size(452, 198);
            this.tableLayoutPanel3.TabIndex = 0;
            // 
            // textboxKeystoreCertCN
            // 
            this.textboxKeystoreCertCN.Location = new System.Drawing.Point(182, 3);
            this.textboxKeystoreCertCN.MaxLength = 48;
            this.textboxKeystoreCertCN.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertCN.Name = "textboxKeystoreCertCN";
            this.textboxKeystoreCertCN.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertCN.TabIndex = 1;
            this.textboxKeystoreCertCN.TextChanged += new System.EventHandler(this.textboxKeystoreCertCN_TextChanged);
            this.textboxKeystoreCertCN.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertCN_Validating);
            // 
            // label17
            // 
            this.label17.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(37, 0);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(139, 17);
            this.label17.TabIndex = 0;
            this.label17.Text = "First and Last Name:";
            // 
            // textboxKeystoreCertC
            // 
            this.textboxKeystoreCertC.Location = new System.Drawing.Point(182, 163);
            this.textboxKeystoreCertC.MaxLength = 2;
            this.textboxKeystoreCertC.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertC.Name = "textboxKeystoreCertC";
            this.textboxKeystoreCertC.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertC.TabIndex = 11;
            this.textboxKeystoreCertC.TextChanged += new System.EventHandler(this.textboxKeystoreCertC_TextChanged);
            this.textboxKeystoreCertC.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertC_Validating);
            // 
            // label22
            // 
            this.label22.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label22.AutoSize = true;
            this.label22.Location = new System.Drawing.Point(46, 160);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(130, 17);
            this.label22.TabIndex = 10;
            this.label22.Text = "Country Code (XX):";
            // 
            // label18
            // 
            this.label18.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(43, 32);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(133, 17);
            this.label18.TabIndex = 2;
            this.label18.Text = "Organizational Unit:";
            // 
            // textboxKeystoreCertOU
            // 
            this.textboxKeystoreCertOU.Location = new System.Drawing.Point(182, 35);
            this.textboxKeystoreCertOU.MaxLength = 48;
            this.textboxKeystoreCertOU.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertOU.Name = "textboxKeystoreCertOU";
            this.textboxKeystoreCertOU.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertOU.TabIndex = 3;
            this.textboxKeystoreCertOU.TextChanged += new System.EventHandler(this.textboxKeystoreCertOU_TextChanged);
            this.textboxKeystoreCertOU.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertOU_Validating);
            // 
            // textboxKeystoreCertST
            // 
            this.textboxKeystoreCertST.Location = new System.Drawing.Point(182, 131);
            this.textboxKeystoreCertST.MaxLength = 48;
            this.textboxKeystoreCertST.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertST.Name = "textboxKeystoreCertST";
            this.textboxKeystoreCertST.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertST.TabIndex = 9;
            this.textboxKeystoreCertST.TextChanged += new System.EventHandler(this.textboxKeystoreCertST_TextChanged);
            this.textboxKeystoreCertST.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertST_Validating);
            // 
            // label21
            // 
            this.label21.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label21.AutoSize = true;
            this.label21.Location = new System.Drawing.Point(55, 128);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(121, 17);
            this.label21.TabIndex = 8;
            this.label21.Text = "State or Province:";
            // 
            // label19
            // 
            this.label19.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(83, 64);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(93, 17);
            this.label19.TabIndex = 4;
            this.label19.Text = "Organization:";
            // 
            // textboxKeystoreCertO
            // 
            this.textboxKeystoreCertO.Location = new System.Drawing.Point(182, 67);
            this.textboxKeystoreCertO.MaxLength = 48;
            this.textboxKeystoreCertO.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertO.Name = "textboxKeystoreCertO";
            this.textboxKeystoreCertO.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertO.TabIndex = 5;
            this.textboxKeystoreCertO.TextChanged += new System.EventHandler(this.textboxKeystoreCertO_TextChanged);
            this.textboxKeystoreCertO.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertO_Validating);
            // 
            // textboxKeystoreCertL
            // 
            this.textboxKeystoreCertL.Location = new System.Drawing.Point(182, 99);
            this.textboxKeystoreCertL.MaxLength = 48;
            this.textboxKeystoreCertL.MinimumSize = new System.Drawing.Size(236, 24);
            this.textboxKeystoreCertL.Name = "textboxKeystoreCertL";
            this.textboxKeystoreCertL.Size = new System.Drawing.Size(236, 22);
            this.textboxKeystoreCertL.TabIndex = 7;
            this.textboxKeystoreCertL.TextChanged += new System.EventHandler(this.textboxKeystoreCertL_TextChanged);
            this.textboxKeystoreCertL.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreCertL_Validating);
            // 
            // label20
            // 
            this.label20.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(72, 96);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(104, 17);
            this.label20.TabIndex = 6;
            this.label20.Text = "City or Locality:";
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 38.07107F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 61.92893F));
            this.tableLayoutPanel1.Controls.Add(this.textboxKeystoreKeyPassword, 1, 1);
            this.tableLayoutPanel1.Controls.Add(this.labelKeystoreKeyAlias, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.label23, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.textboxKeystoreKeyAlias, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.numericValidityYears, 1, 2);
            this.tableLayoutPanel1.Controls.Add(this.labelKeystoreKeyPass, 0, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 18);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 33.33333F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(458, 100);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // textboxKeystoreKeyPassword
            // 
            this.textboxKeystoreKeyPassword.Location = new System.Drawing.Point(177, 36);
            this.textboxKeystoreKeyPassword.MaxLength = 64;
            this.textboxKeystoreKeyPassword.MinimumSize = new System.Drawing.Size(238, 24);
            this.textboxKeystoreKeyPassword.Name = "textboxKeystoreKeyPassword";
            this.textboxKeystoreKeyPassword.Size = new System.Drawing.Size(238, 22);
            this.textboxKeystoreKeyPassword.TabIndex = 20;
            this.textboxKeystoreKeyPassword.TextChanged += new System.EventHandler(this.textboxKeystoreKeyPassword_TextChanged);
            this.textboxKeystoreKeyPassword.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreKeyPassword_Validating);
            // 
            // labelKeystoreKeyAlias
            // 
            this.labelKeystoreKeyAlias.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelKeystoreKeyAlias.AutoSize = true;
            this.labelKeystoreKeyAlias.Location = new System.Drawing.Point(101, 0);
            this.labelKeystoreKeyAlias.Name = "labelKeystoreKeyAlias";
            this.labelKeystoreKeyAlias.Size = new System.Drawing.Size(70, 17);
            this.labelKeystoreKeyAlias.TabIndex = 15;
            this.labelKeystoreKeyAlias.Text = "Key Alias:";
            // 
            // label23
            // 
            this.label23.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label23.AutoSize = true;
            this.label23.Location = new System.Drawing.Point(65, 66);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(106, 17);
            this.label23.TabIndex = 12;
            this.label23.Text = "Validity (years):";
            // 
            // textboxKeystoreKeyAlias
            // 
            this.textboxKeystoreKeyAlias.Location = new System.Drawing.Point(177, 3);
            this.textboxKeystoreKeyAlias.MaxLength = 64;
            this.textboxKeystoreKeyAlias.MinimumSize = new System.Drawing.Size(238, 24);
            this.textboxKeystoreKeyAlias.Name = "textboxKeystoreKeyAlias";
            this.textboxKeystoreKeyAlias.Size = new System.Drawing.Size(238, 22);
            this.textboxKeystoreKeyAlias.TabIndex = 19;
            this.textboxKeystoreKeyAlias.TextChanged += new System.EventHandler(this.textboxKeystoreKeyAlias_TextChanged);
            this.textboxKeystoreKeyAlias.Validating += new System.ComponentModel.CancelEventHandler(this.textboxKeystoreKeyAlias_Validating);
            // 
            // numericValidityYears
            // 
            this.numericValidityYears.Location = new System.Drawing.Point(177, 69);
            this.numericValidityYears.Name = "numericValidityYears";
            this.numericValidityYears.Size = new System.Drawing.Size(120, 22);
            this.numericValidityYears.TabIndex = 3;
            this.numericValidityYears.Validating += new System.ComponentModel.CancelEventHandler(this.numericValidityYears_Validating);
            // 
            // labelKeystoreKeyPass
            // 
            this.labelKeystoreKeyPass.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labelKeystoreKeyPass.AutoSize = true;
            this.labelKeystoreKeyPass.Location = new System.Drawing.Point(70, 33);
            this.labelKeystoreKeyPass.Name = "labelKeystoreKeyPass";
            this.labelKeystoreKeyPass.Size = new System.Drawing.Size(101, 17);
            this.labelKeystoreKeyPass.TabIndex = 16;
            this.labelKeystoreKeyPass.Text = "Key Password:";
            // 
            // richTextBox1
            // 
            this.richTextBox1.CausesValidation = false;
            this.richTextBox1.DetectUrls = false;
            this.richTextBox1.Enabled = false;
            this.richTextBox1.HideSelection = false;
            this.richTextBox1.Location = new System.Drawing.Point(473, 3);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.ReadOnly = true;
            this.richTextBox1.Size = new System.Drawing.Size(302, 404);
            this.richTextBox1.TabIndex = 6;
            this.richTextBox1.Text = resources.GetString("richTextBox1.Text");
            // 
            // GenerateAndroidKeystore
            // 
            this.AcceptButton = this.buttonOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(818, 514);
            this.Controls.Add(this.flowLayoutPanel1);
            this.Controls.Add(this.panel1);
            this.MinimizeBox = false;
            this.Name = "GenerateAndroidKeystore";
            this.ShowIcon = false;
            this.Text = "GenerateAndroidKeystore";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.flowLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.groupBoxKey.ResumeLayout(false);
            this.groupBoxKey.PerformLayout();
            this.groupBoxCertificate.ResumeLayout(false);
            this.tableLayoutPanel3.ResumeLayout(false);
            this.tableLayoutPanel3.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numericValidityYears)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.Label labelExplanation;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.TextBox textboxKeystorePassword;
        private System.Windows.Forms.TextBox textboxKeystorePath;
        private System.Windows.Forms.Label labelKeystore_Path;
        private System.Windows.Forms.Label labelKeystorePass;
        private System.Windows.Forms.GroupBox groupBoxKey;
        private System.Windows.Forms.GroupBox groupBoxCertificate;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel3;
        private System.Windows.Forms.TextBox textboxKeystoreCertCN;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.TextBox textboxKeystoreCertC;
        private System.Windows.Forms.Label label22;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.TextBox textboxKeystoreCertOU;
        private System.Windows.Forms.TextBox textboxKeystoreCertST;
        private System.Windows.Forms.Label label21;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.TextBox textboxKeystoreCertO;
        private System.Windows.Forms.TextBox textboxKeystoreCertL;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TextBox textboxKeystoreKeyPassword;
        private System.Windows.Forms.Label labelKeystoreKeyAlias;
        private System.Windows.Forms.Label label23;
        private System.Windows.Forms.TextBox textboxKeystoreKeyAlias;
        private System.Windows.Forms.NumericUpDown numericValidityYears;
        private System.Windows.Forms.Label labelKeystoreKeyPass;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private System.Windows.Forms.Button buttonGenerate;
        private System.Windows.Forms.Button buttonOK;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.RichTextBox richTextBox1;
        private System.Windows.Forms.Button buttonBrowse;
    }
}