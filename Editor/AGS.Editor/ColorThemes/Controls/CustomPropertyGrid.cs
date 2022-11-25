using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

// The only purpose of this class is to let us theme the toolstrip
namespace AGS.Editor
{
    public class CustomPropertyGrid : PropertyGrid
    {
        public CustomPropertyGrid() : base()
        {
        }

        public void SetRenderer(ToolStripRenderer renderer)
        {
            ToolStripRenderer = renderer;
        }
    }
}
