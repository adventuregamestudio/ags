using System;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class ImportTTFDialog : Form
    {
        public ImportTTFDialog(FontHeightDefinition sizeType, int sizeValue, int maxSize)
        {
            InitializeComponent();
            rbNominalSize.Checked = sizeType == FontHeightDefinition.NominalHeight;
            rbRealPixelHeight.Checked = sizeType == FontHeightDefinition.PixelHeight;
            udSize.Enabled = rbNominalSize.Checked;
            udFullHeight.Enabled = rbRealPixelHeight.Checked;
            udSize.Value = sizeValue;
            udSize.Minimum = 1;
            udSize.Maximum = maxSize;
            udFullHeight.Value = sizeValue;
            udFullHeight.Minimum = 1;
            udFullHeight.Maximum = maxSize;
            ActiveControl = udSize;
        }

        public FontHeightDefinition SizeType
        {
            get
            {
                return rbNominalSize.Checked ?
                  FontHeightDefinition.NominalHeight : FontHeightDefinition.PixelHeight;
            }
        }

        public int SizeValue
        {
            get { return rbNominalSize.Checked ? (int)udSize.Value : (int)udFullHeight.Value; }
        }

        public static bool Show(out FontHeightDefinition sizeType, out int sizeValue, int currentSize, int maxSize)
        {
            ImportTTFDialog dialog = new ImportTTFDialog(FontHeightDefinition.NominalHeight, currentSize, maxSize);
            bool result = dialog.ShowDialog() == DialogResult.OK;
            sizeType = dialog.SizeType;
            sizeValue = dialog.SizeValue;
            return result;
        }

        private void rb_CheckedChanged(object sender, EventArgs e)
        {
            udSize.Enabled = rbNominalSize.Checked;
            udFullHeight.Enabled = rbRealPixelHeight.Checked;
        }
    }
}
