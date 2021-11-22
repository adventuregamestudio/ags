using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ImportTTFDialog : Form
    {
        public enum FontSizeMeaning
        {
            PointSize,
            RealPixelHeight
        }

        public ImportTTFDialog(FontSizeMeaning sizeType, int sizeValue, int maxSize)
        {
            InitializeComponent();
            rbPointSize.Checked = sizeType == FontSizeMeaning.PointSize;
            rbRealPixelHeight.Checked = sizeType == FontSizeMeaning.RealPixelHeight;
            udSize.Value = sizeValue;
            udSize.Minimum = 1;
            udSize.Maximum = maxSize;
            ActiveControl = udSize;
        }

        public FontSizeMeaning SizeType
        {
            get
            {
                return rbPointSize.Checked ?
                  FontSizeMeaning.PointSize : FontSizeMeaning.RealPixelHeight;
            }
        }

        public int SizeValue
        {
            get { return (int)udSize.Value; }
        }

        public static bool Show(out FontSizeMeaning sizeType, out int sizeValue, int currentSize, int maxSize)
        {
            ImportTTFDialog dialog = new ImportTTFDialog(FontSizeMeaning.PointSize, currentSize, maxSize);
            bool result = dialog.ShowDialog() == DialogResult.OK;
            sizeType = dialog.SizeType;
            sizeValue = dialog.SizeValue;
            return result;
        }
    }
}
