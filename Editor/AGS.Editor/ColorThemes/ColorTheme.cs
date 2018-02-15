using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public abstract class ColorTheme
    {
        protected ColorTheme(string name)
        {
            Name = name;
        }

        public string Name { get; }

        public abstract void Init();

        public abstract Color GetColor(string id);

        public abstract int GetInt(string id);

        public abstract bool GetBool(string id);

        public abstract ToolStripRenderer GetMainMenuRenderer(string id);

        public abstract ToolStripRenderer GetToolStripRenderer(string id);

        public abstract ComboBox GetComboBox(string id, ComboBox original);

        public abstract Image GetImage(string id, Image original);

        public override string ToString() => Name;
    }
}
