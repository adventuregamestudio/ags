using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class AdjustMasksDialog : Form
    {
        private AdjustMaskOptions _maskOption = AdjustMaskOptions.ResetMask;
        private int _xOffset = 0;
        private int _yOffset = 0;

        public AdjustMasksDialog()
        {
            InitializeComponent();
        }

        public AdjustMaskOptions MaskOption
        {
            get { return _maskOption; }
        }

        public int XOffset
        {
            get { return _xOffset; }
        }

        public int YOffset
        {
            get { return _yOffset; }
        }


        private void btnOK_Click(object sender, EventArgs e)
        {
            if (radioResize.Checked)
            {
                _maskOption = AdjustMaskOptions.ResizeMaskCanvas;
            }
            else if (radioScale.Checked)
            {
                _maskOption = AdjustMaskOptions.ScaleMaskImage;
            }
            else
            {
                _maskOption = AdjustMaskOptions.ResetMask;
            }

            _xOffset = (int)xOffset.Value;
            _yOffset = (int)yOffset.Value;

            this.DialogResult = DialogResult.OK;
        }
    }
}
