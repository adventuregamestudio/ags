using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace AGS.Types.PropertyGridExtras
{
    public class SpriteFileNameEditor : FileNameEditor
    {
        protected override void InitializeDialog(OpenFileDialog openFileDialog)
        {
            base.InitializeDialog(openFileDialog);
            openFileDialog.Multiselect = false;
            openFileDialog.Filter = Constants.IMAGE_FILE_FILTER;
        }
    }
}
