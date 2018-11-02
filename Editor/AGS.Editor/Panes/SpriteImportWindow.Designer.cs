namespace AGS.Editor
{
    partial class SpriteImportWindow
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SpriteImportWindow));
            this.groupImportOptions = new System.Windows.Forms.GroupBox();
            this.chkUseAlphaChannel = new System.Windows.Forms.CheckBox();
            this.chkRoomBackground = new System.Windows.Forms.CheckBox();
            this.chkRemapCols = new System.Windows.Forms.CheckBox();
            this.groupTransColour = new System.Windows.Forms.GroupBox();
            this.panelBottomRight = new System.Windows.Forms.Panel();
            this.panelTopRight = new System.Windows.Forms.Panel();
            this.panelBottomLeft = new System.Windows.Forms.Panel();
            this.panelIndex0 = new System.Windows.Forms.Panel();
            this.panelTopLeft = new System.Windows.Forms.Panel();
            this.radTransColourNone = new System.Windows.Forms.RadioButton();
            this.radTransColourLeaveAsIs = new System.Windows.Forms.RadioButton();
            this.radTransColourBottomRightPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourTopRightPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourBottomLeftPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourTopLeftPixel = new System.Windows.Forms.RadioButton();
            this.radTransColourIndex0 = new System.Windows.Forms.RadioButton();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnImport = new System.Windows.Forms.Button();
            this.chkTiled = new System.Windows.Forms.CheckBox();
            this.cmbFilenames = new System.Windows.Forms.ComboBox();
            this.lblImageDescription = new System.Windows.Forms.Label();
            this.zoomSlider = new System.Windows.Forms.TrackBar();
            this.lblZoom = new System.Windows.Forms.Label();
            this.groupSelection = new System.Windows.Forms.GroupBox();
            this.lblX = new System.Windows.Forms.Label();
            this.lblMargin = new System.Windows.Forms.Label();
            this.numMarginX = new System.Windows.Forms.NumericUpDown();
            this.numMarginY = new System.Windows.Forms.NumericUpDown();
            this.lblTileDirection = new System.Windows.Forms.Label();
            this.cmbTileDirection = new System.Windows.Forms.ComboBox();
            this.lblMaxTiles = new System.Windows.Forms.Label();
            this.numMaxTiles = new System.Windows.Forms.NumericUpDown();
            this.lblSize = new System.Windows.Forms.Label();
            this.lblOffset = new System.Windows.Forms.Label();
            this.numSizeY = new System.Windows.Forms.NumericUpDown();
            this.numSizeX = new System.Windows.Forms.NumericUpDown();
            this.numOffsetX = new System.Windows.Forms.NumericUpDown();
            this.numOffsetY = new System.Windows.Forms.NumericUpDown();
            this.previewPanel = new AGS.Editor.BufferedPanel();
            this.lblY = new System.Windows.Forms.Label();
            this.groupImportOptions.SuspendLayout();
            this.groupTransColour.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).BeginInit();
            this.groupSelection.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxTiles)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeY)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetX)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetY)).BeginInit();
            this.SuspendLayout();
            // 
            // groupImportOptions
            // 
            this.groupImportOptions.Controls.Add(this.chkUseAlphaChannel);
            this.groupImportOptions.Controls.Add(this.chkRoomBackground);
            this.groupImportOptions.Controls.Add(this.chkRemapCols);
            this.groupImportOptions.Location = new System.Drawing.Point(12, 12);
            this.groupImportOptions.Name = "groupImportOptions";
            this.groupImportOptions.Size = new System.Drawing.Size(202, 90);
            this.groupImportOptions.TabIndex = 3;
            this.groupImportOptions.TabStop = false;
            this.groupImportOptions.Text = "Import options";
            // 
            // chkUseAlphaChannel
            // 
            this.chkUseAlphaChannel.AutoSize = true;
            this.chkUseAlphaChannel.Checked = true;
            this.chkUseAlphaChannel.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkUseAlphaChannel.Location = new System.Drawing.Point(6, 20);
            this.chkUseAlphaChannel.Name = "chkUseAlphaChannel";
            this.chkUseAlphaChannel.Size = new System.Drawing.Size(189, 17);
            this.chkUseAlphaChannel.TabIndex = 12;
            this.chkUseAlphaChannel.Text = "Import alpha channel (if available)";
            this.chkUseAlphaChannel.UseVisualStyleBackColor = true;
            // 
            // chkRoomBackground
            // 
            this.chkRoomBackground.AutoSize = true;
            this.chkRoomBackground.Location = new System.Drawing.Point(6, 66);
            this.chkRoomBackground.Name = "chkRoomBackground";
            this.chkRoomBackground.Size = new System.Drawing.Size(183, 17);
            this.chkRoomBackground.TabIndex = 4;
            this.chkRoomBackground.Text = "Lock to room background palette";
            this.chkRoomBackground.UseVisualStyleBackColor = true;
            // 
            // chkRemapCols
            // 
            this.chkRemapCols.AutoSize = true;
            this.chkRemapCols.Checked = true;
            this.chkRemapCols.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkRemapCols.Location = new System.Drawing.Point(6, 43);
            this.chkRemapCols.Name = "chkRemapCols";
            this.chkRemapCols.Size = new System.Drawing.Size(175, 17);
            this.chkRemapCols.TabIndex = 3;
            this.chkRemapCols.Text = "Remap colours to game palette";
            this.chkRemapCols.UseVisualStyleBackColor = true;
            // 
            // groupTransColour
            // 
            this.groupTransColour.Controls.Add(this.panelBottomRight);
            this.groupTransColour.Controls.Add(this.panelTopRight);
            this.groupTransColour.Controls.Add(this.panelBottomLeft);
            this.groupTransColour.Controls.Add(this.panelIndex0);
            this.groupTransColour.Controls.Add(this.panelTopLeft);
            this.groupTransColour.Controls.Add(this.radTransColourNone);
            this.groupTransColour.Controls.Add(this.radTransColourLeaveAsIs);
            this.groupTransColour.Controls.Add(this.radTransColourBottomRightPixel);
            this.groupTransColour.Controls.Add(this.radTransColourTopRightPixel);
            this.groupTransColour.Controls.Add(this.radTransColourBottomLeftPixel);
            this.groupTransColour.Controls.Add(this.radTransColourTopLeftPixel);
            this.groupTransColour.Controls.Add(this.radTransColourIndex0);
            this.groupTransColour.Location = new System.Drawing.Point(12, 108);
            this.groupTransColour.Name = "groupTransColour";
            this.groupTransColour.Size = new System.Drawing.Size(202, 192);
            this.groupTransColour.TabIndex = 14;
            this.groupTransColour.TabStop = false;
            this.groupTransColour.Text = "Transparent colour";
            // 
            // panelBottomRight
            // 
            this.panelBottomRight.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelBottomRight.Location = new System.Drawing.Point(134, 158);
            this.panelBottomRight.Name = "panelBottomRight";
            this.panelBottomRight.Size = new System.Drawing.Size(62, 17);
            this.panelBottomRight.TabIndex = 21;
            // 
            // panelTopRight
            // 
            this.panelTopRight.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelTopRight.Location = new System.Drawing.Point(134, 135);
            this.panelTopRight.Name = "panelTopRight";
            this.panelTopRight.Size = new System.Drawing.Size(62, 17);
            this.panelTopRight.TabIndex = 21;
            // 
            // panelBottomLeft
            // 
            this.panelBottomLeft.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelBottomLeft.Location = new System.Drawing.Point(134, 112);
            this.panelBottomLeft.Name = "panelBottomLeft";
            this.panelBottomLeft.Size = new System.Drawing.Size(62, 17);
            this.panelBottomLeft.TabIndex = 21;
            // 
            // panelIndex0
            // 
            this.panelIndex0.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelIndex0.Location = new System.Drawing.Point(134, 66);
            this.panelIndex0.Name = "panelIndex0";
            this.panelIndex0.Size = new System.Drawing.Size(62, 17);
            this.panelIndex0.TabIndex = 21;
            // 
            // panelTopLeft
            // 
            this.panelTopLeft.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panelTopLeft.Location = new System.Drawing.Point(134, 89);
            this.panelTopLeft.Name = "panelTopLeft";
            this.panelTopLeft.Size = new System.Drawing.Size(62, 17);
            this.panelTopLeft.TabIndex = 20;
            // 
            // radTransColourNone
            // 
            this.radTransColourNone.AutoSize = true;
            this.radTransColourNone.Location = new System.Drawing.Point(6, 43);
            this.radTransColourNone.Name = "radTransColourNone";
            this.radTransColourNone.Size = new System.Drawing.Size(105, 17);
            this.radTransColourNone.TabIndex = 19;
            this.radTransColourNone.Text = "No transparency";
            this.radTransColourNone.UseVisualStyleBackColor = true;
            // 
            // radTransColourLeaveAsIs
            // 
            this.radTransColourLeaveAsIs.AutoSize = true;
            this.radTransColourLeaveAsIs.Checked = true;
            this.radTransColourLeaveAsIs.Location = new System.Drawing.Point(6, 20);
            this.radTransColourLeaveAsIs.Name = "radTransColourLeaveAsIs";
            this.radTransColourLeaveAsIs.Size = new System.Drawing.Size(79, 17);
            this.radTransColourLeaveAsIs.TabIndex = 18;
            this.radTransColourLeaveAsIs.TabStop = true;
            this.radTransColourLeaveAsIs.Text = "Leave as-is";
            this.radTransColourLeaveAsIs.UseVisualStyleBackColor = true;
            // 
            // radTransColourBottomRightPixel
            // 
            this.radTransColourBottomRightPixel.AutoSize = true;
            this.radTransColourBottomRightPixel.Location = new System.Drawing.Point(5, 158);
            this.radTransColourBottomRightPixel.Name = "radTransColourBottomRightPixel";
            this.radTransColourBottomRightPixel.Size = new System.Drawing.Size(110, 17);
            this.radTransColourBottomRightPixel.TabIndex = 17;
            this.radTransColourBottomRightPixel.Text = "Bottom-right pixel";
            this.radTransColourBottomRightPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourTopRightPixel
            // 
            this.radTransColourTopRightPixel.AutoSize = true;
            this.radTransColourTopRightPixel.Location = new System.Drawing.Point(5, 135);
            this.radTransColourTopRightPixel.Name = "radTransColourTopRightPixel";
            this.radTransColourTopRightPixel.Size = new System.Drawing.Size(94, 17);
            this.radTransColourTopRightPixel.TabIndex = 16;
            this.radTransColourTopRightPixel.Text = "Top-right pixel";
            this.radTransColourTopRightPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourBottomLeftPixel
            // 
            this.radTransColourBottomLeftPixel.AutoSize = true;
            this.radTransColourBottomLeftPixel.Location = new System.Drawing.Point(5, 112);
            this.radTransColourBottomLeftPixel.Name = "radTransColourBottomLeftPixel";
            this.radTransColourBottomLeftPixel.Size = new System.Drawing.Size(104, 17);
            this.radTransColourBottomLeftPixel.TabIndex = 15;
            this.radTransColourBottomLeftPixel.Text = "Bottom-left pixel";
            this.radTransColourBottomLeftPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourTopLeftPixel
            // 
            this.radTransColourTopLeftPixel.AutoSize = true;
            this.radTransColourTopLeftPixel.Location = new System.Drawing.Point(5, 89);
            this.radTransColourTopLeftPixel.Name = "radTransColourTopLeftPixel";
            this.radTransColourTopLeftPixel.Size = new System.Drawing.Size(88, 17);
            this.radTransColourTopLeftPixel.TabIndex = 14;
            this.radTransColourTopLeftPixel.Text = "Top-left pixel";
            this.radTransColourTopLeftPixel.UseVisualStyleBackColor = true;
            // 
            // radTransColourIndex0
            // 
            this.radTransColourIndex0.AutoSize = true;
            this.radTransColourIndex0.Location = new System.Drawing.Point(5, 66);
            this.radTransColourIndex0.Name = "radTransColourIndex0";
            this.radTransColourIndex0.Size = new System.Drawing.Size(97, 17);
            this.radTransColourIndex0.TabIndex = 13;
            this.radTransColourIndex0.Text = "Palette index 0";
            this.radTransColourIndex0.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(12, 519);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(70, 30);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnImport
            // 
            this.btnImport.Location = new System.Drawing.Point(88, 519);
            this.btnImport.Name = "btnImport";
            this.btnImport.Size = new System.Drawing.Size(70, 30);
            this.btnImport.TabIndex = 11;
            this.btnImport.Text = "Import";
            this.btnImport.UseVisualStyleBackColor = true;
            this.btnImport.Click += new System.EventHandler(this.btnImport_Click);
            // 
            // chkTiled
            // 
            this.chkTiled.AutoSize = true;
            this.chkTiled.Location = new System.Drawing.Point(63, 20);
            this.chkTiled.Name = "chkTiled";
            this.chkTiled.Size = new System.Drawing.Size(111, 17);
            this.chkTiled.TabIndex = 9;
            this.chkTiled.Text = "Tiled sprite import";
            this.chkTiled.UseVisualStyleBackColor = true;
            this.chkTiled.CheckedChanged += new System.EventHandler(this.chkTiled_CheckedChanged);
            // 
            // cmbFilenames
            // 
            this.cmbFilenames.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.cmbFilenames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbFilenames.FormattingEnabled = true;
            this.cmbFilenames.Location = new System.Drawing.Point(552, 12);
            this.cmbFilenames.Name = "cmbFilenames";
            this.cmbFilenames.Size = new System.Drawing.Size(220, 21);
            this.cmbFilenames.TabIndex = 6;
            this.cmbFilenames.SelectedIndexChanged += new System.EventHandler(this.cmbFilenames_SelectedIndexChanged);
            // 
            // lblImageDescription
            // 
            this.lblImageDescription.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblImageDescription.AutoSize = true;
            this.lblImageDescription.Location = new System.Drawing.Point(552, 44);
            this.lblImageDescription.Name = "lblImageDescription";
            this.lblImageDescription.Size = new System.Drawing.Size(69, 13);
            this.lblImageDescription.TabIndex = 7;
            this.lblImageDescription.Text = "image details";
            // 
            // zoomSlider
            // 
            this.zoomSlider.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.zoomSlider.LargeChange = 2;
            this.zoomSlider.Location = new System.Drawing.Point(223, 12);
            this.zoomSlider.Maximum = 15;
            this.zoomSlider.Minimum = 1;
            this.zoomSlider.Name = "zoomSlider";
            this.zoomSlider.Size = new System.Drawing.Size(323, 45);
            this.zoomSlider.TabIndex = 4;
            this.zoomSlider.Value = 1;
            this.zoomSlider.Scroll += new System.EventHandler(this.zoomSlider_Scroll);
            // 
            // lblZoom
            // 
            this.lblZoom.AutoSize = true;
            this.lblZoom.Location = new System.Drawing.Point(220, 44);
            this.lblZoom.Name = "lblZoom";
            this.lblZoom.Size = new System.Drawing.Size(55, 13);
            this.lblZoom.TabIndex = 5;
            this.lblZoom.Text = "Zoom: x 1";
            // 
            // groupSelection
            // 
            this.groupSelection.Controls.Add(this.cmbTileDirection);
            this.groupSelection.Controls.Add(this.numMaxTiles);
            this.groupSelection.Controls.Add(this.lblY);
            this.groupSelection.Controls.Add(this.lblMaxTiles);
            this.groupSelection.Controls.Add(this.lblX);
            this.groupSelection.Controls.Add(this.lblTileDirection);
            this.groupSelection.Controls.Add(this.lblMargin);
            this.groupSelection.Controls.Add(this.numMarginX);
            this.groupSelection.Controls.Add(this.numMarginY);
            this.groupSelection.Controls.Add(this.lblSize);
            this.groupSelection.Controls.Add(this.lblOffset);
            this.groupSelection.Controls.Add(this.numSizeY);
            this.groupSelection.Controls.Add(this.numSizeX);
            this.groupSelection.Controls.Add(this.numOffsetX);
            this.groupSelection.Controls.Add(this.numOffsetY);
            this.groupSelection.Controls.Add(this.chkTiled);
            this.groupSelection.Location = new System.Drawing.Point(12, 306);
            this.groupSelection.Name = "groupSelection";
            this.groupSelection.Size = new System.Drawing.Size(200, 207);
            this.groupSelection.TabIndex = 15;
            this.groupSelection.TabStop = false;
            this.groupSelection.Text = "Selection";
            // 
            // lblX
            // 
            this.lblX.AutoSize = true;
            this.lblX.Location = new System.Drawing.Point(63, 102);
            this.lblX.Name = "lblX";
            this.lblX.Size = new System.Drawing.Size(13, 13);
            this.lblX.TabIndex = 23;
            this.lblX.Text = "X";
            // 
            // lblMargin
            // 
            this.lblMargin.AutoSize = true;
            this.lblMargin.Location = new System.Drawing.Point(18, 178);
            this.lblMargin.Name = "lblMargin";
            this.lblMargin.Size = new System.Drawing.Size(39, 13);
            this.lblMargin.TabIndex = 22;
            this.lblMargin.Text = "Margin";
            // 
            // numMarginX
            // 
            this.numMarginX.Enabled = false;
            this.numMarginX.Location = new System.Drawing.Point(63, 176);
            this.numMarginX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMarginX.Name = "numMarginX";
            this.numMarginX.Size = new System.Drawing.Size(62, 21);
            this.numMarginX.TabIndex = 21;
            this.numMarginX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMarginX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numMarginY
            // 
            this.numMarginY.Enabled = false;
            this.numMarginY.Location = new System.Drawing.Point(131, 176);
            this.numMarginY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMarginY.Name = "numMarginY";
            this.numMarginY.Size = new System.Drawing.Size(62, 21);
            this.numMarginY.TabIndex = 20;
            this.numMarginY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMarginY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // lblTileDirection
            // 
            this.lblTileDirection.AutoSize = true;
            this.lblTileDirection.Location = new System.Drawing.Point(10, 46);
            this.lblTileDirection.Name = "lblTileDirection";
            this.lblTileDirection.Size = new System.Drawing.Size(49, 13);
            this.lblTileDirection.TabIndex = 19;
            this.lblTileDirection.Text = "Direction";
            // 
            // cmbTileDirection
            // 
            this.cmbTileDirection.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbTileDirection.Enabled = false;
            this.cmbTileDirection.FormattingEnabled = true;
            this.cmbTileDirection.Items.AddRange(new object[] {
            "To the right",
            "Downwards"});
            this.cmbTileDirection.Location = new System.Drawing.Point(63, 43);
            this.cmbTileDirection.Name = "cmbTileDirection";
            this.cmbTileDirection.Size = new System.Drawing.Size(131, 21);
            this.cmbTileDirection.TabIndex = 18;
            this.cmbTileDirection.SelectedIndexChanged += new System.EventHandler(this.cmbTileDirection_SelectedIndexChanged);
            // 
            // lblMaxTiles
            // 
            this.lblMaxTiles.AutoSize = true;
            this.lblMaxTiles.Location = new System.Drawing.Point(10, 76);
            this.lblMaxTiles.Name = "lblMaxTiles";
            this.lblMaxTiles.Size = new System.Drawing.Size(79, 13);
            this.lblMaxTiles.TabIndex = 17;
            this.lblMaxTiles.Text = "Number of tiles";
            // 
            // numMaxTiles
            // 
            this.numMaxTiles.Enabled = false;
            this.numMaxTiles.Location = new System.Drawing.Point(131, 74);
            this.numMaxTiles.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numMaxTiles.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxTiles.Name = "numMaxTiles";
            this.numMaxTiles.Size = new System.Drawing.Size(63, 21);
            this.numMaxTiles.TabIndex = 16;
            this.numMaxTiles.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numMaxTiles.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numMaxTiles.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // lblSize
            // 
            this.lblSize.AutoSize = true;
            this.lblSize.Location = new System.Drawing.Point(31, 150);
            this.lblSize.Name = "lblSize";
            this.lblSize.Size = new System.Drawing.Size(26, 13);
            this.lblSize.TabIndex = 15;
            this.lblSize.Text = "Size";
            // 
            // lblOffset
            // 
            this.lblOffset.AutoSize = true;
            this.lblOffset.Location = new System.Drawing.Point(20, 123);
            this.lblOffset.Name = "lblOffset";
            this.lblOffset.Size = new System.Drawing.Size(38, 13);
            this.lblOffset.TabIndex = 14;
            this.lblOffset.Text = "Offset";
            // 
            // numSizeY
            // 
            this.numSizeY.Enabled = false;
            this.numSizeY.Location = new System.Drawing.Point(132, 148);
            this.numSizeY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numSizeY.Name = "numSizeY";
            this.numSizeY.Size = new System.Drawing.Size(62, 21);
            this.numSizeY.TabIndex = 13;
            this.numSizeY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numSizeY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numSizeX
            // 
            this.numSizeX.Enabled = false;
            this.numSizeX.Location = new System.Drawing.Point(64, 148);
            this.numSizeX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numSizeX.Name = "numSizeX";
            this.numSizeX.Size = new System.Drawing.Size(62, 21);
            this.numSizeX.TabIndex = 12;
            this.numSizeX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numSizeX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numOffsetX
            // 
            this.numOffsetX.Enabled = false;
            this.numOffsetX.Location = new System.Drawing.Point(64, 121);
            this.numOffsetX.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numOffsetX.Name = "numOffsetX";
            this.numOffsetX.Size = new System.Drawing.Size(62, 21);
            this.numOffsetX.TabIndex = 11;
            this.numOffsetX.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numOffsetX.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // numOffsetY
            // 
            this.numOffsetY.Enabled = false;
            this.numOffsetY.Location = new System.Drawing.Point(132, 121);
            this.numOffsetY.Maximum = new decimal(new int[] {
            99999,
            0,
            0,
            0});
            this.numOffsetY.Name = "numOffsetY";
            this.numOffsetY.Size = new System.Drawing.Size(62, 21);
            this.numOffsetY.TabIndex = 10;
            this.numOffsetY.ValueChanged += new System.EventHandler(this.InvalidateOn_ValueChanged);
            this.numOffsetY.KeyUp += new System.Windows.Forms.KeyEventHandler(this.InvalidateOn_KeyUp);
            // 
            // previewPanel
            // 
            this.previewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.previewPanel.AutoScroll = true;
            this.previewPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.previewPanel.Location = new System.Drawing.Point(223, 66);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(549, 483);
            this.previewPanel.TabIndex = 0;
            this.previewPanel.Scroll += new System.Windows.Forms.ScrollEventHandler(this.previewPanel_Scroll);
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            this.previewPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseDown);
            this.previewPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseMove);
            this.previewPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseUp);
            // 
            // lblY
            // 
            this.lblY.AutoSize = true;
            this.lblY.Location = new System.Drawing.Point(132, 102);
            this.lblY.Name = "lblY";
            this.lblY.Size = new System.Drawing.Size(13, 13);
            this.lblY.TabIndex = 24;
            this.lblY.Text = "Y";
            // 
            // SpriteImportWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(784, 561);
            this.Controls.Add(this.groupSelection);
            this.Controls.Add(this.btnImport);
            this.Controls.Add(this.groupTransColour);
            this.Controls.Add(this.lblImageDescription);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.cmbFilenames);
            this.Controls.Add(this.lblZoom);
            this.Controls.Add(this.groupImportOptions);
            this.Controls.Add(this.zoomSlider);
            this.Controls.Add(this.previewPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(800, 600);
            this.Name = "SpriteImportWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Import Sprite";
            this.groupImportOptions.ResumeLayout(false);
            this.groupImportOptions.PerformLayout();
            this.groupTransColour.ResumeLayout(false);
            this.groupTransColour.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).EndInit();
            this.groupSelection.ResumeLayout(false);
            this.groupSelection.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMarginY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numMaxTiles)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeY)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSizeX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetX)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numOffsetY)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private BufferedPanel previewPanel;
        private System.Windows.Forms.GroupBox groupImportOptions;
        private System.Windows.Forms.Label lblImageDescription;
        private System.Windows.Forms.CheckBox chkRoomBackground;
        private System.Windows.Forms.CheckBox chkRemapCols;
        private System.Windows.Forms.CheckBox chkTiled;
        private System.Windows.Forms.TrackBar zoomSlider;
        private System.Windows.Forms.Label lblZoom;
        private System.Windows.Forms.Button btnImport;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ComboBox cmbFilenames;
        private System.Windows.Forms.CheckBox chkUseAlphaChannel;
        private System.Windows.Forms.GroupBox groupTransColour;
        private System.Windows.Forms.RadioButton radTransColourNone;
        private System.Windows.Forms.RadioButton radTransColourLeaveAsIs;
        private System.Windows.Forms.RadioButton radTransColourBottomRightPixel;
        private System.Windows.Forms.RadioButton radTransColourTopRightPixel;
        private System.Windows.Forms.RadioButton radTransColourBottomLeftPixel;
        private System.Windows.Forms.RadioButton radTransColourTopLeftPixel;
        private System.Windows.Forms.RadioButton radTransColourIndex0;
        private System.Windows.Forms.Panel panelBottomRight;
        private System.Windows.Forms.Panel panelTopRight;
        private System.Windows.Forms.Panel panelBottomLeft;
        private System.Windows.Forms.Panel panelIndex0;
        private System.Windows.Forms.Panel panelTopLeft;
        private System.Windows.Forms.GroupBox groupSelection;
        private System.Windows.Forms.NumericUpDown numSizeY;
        private System.Windows.Forms.NumericUpDown numSizeX;
        private System.Windows.Forms.NumericUpDown numOffsetX;
        private System.Windows.Forms.NumericUpDown numOffsetY;
        private System.Windows.Forms.Label lblTileDirection;
        private System.Windows.Forms.ComboBox cmbTileDirection;
        private System.Windows.Forms.Label lblMaxTiles;
        private System.Windows.Forms.NumericUpDown numMaxTiles;
        private System.Windows.Forms.Label lblSize;
        private System.Windows.Forms.Label lblOffset;
        private System.Windows.Forms.Label lblMargin;
        private System.Windows.Forms.NumericUpDown numMarginX;
        private System.Windows.Forms.NumericUpDown numMarginY;
        private System.Windows.Forms.Label lblX;
        private System.Windows.Forms.Label lblY;
    }
}