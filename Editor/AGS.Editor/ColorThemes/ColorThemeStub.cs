using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class ColorThemeStub : ColorTheme
    {
        public static readonly ColorTheme DEFAULT = new ColorThemeStub("Default");

        private ColorThemeStub(string name) : base(name)
        {
        }

        public override void Init()
        {
        }

        public override Color GetColor(string id)
        {
            throw new NotImplementedException();
        }

        public override int GetInt(string id)
        {
            throw new NotImplementedException();
        }

        public override bool GetBool(string id)
        {
            throw new NotImplementedException();
        }

        public override ToolStripRenderer GetMainMenuRenderer(string id)
        {
            throw new NotImplementedException();
        }

        public override ToolStripRenderer GetToolStripRenderer(string id)
        {
            throw new NotImplementedException();
        }

        public override ComboBox GetComboBox(string id, ComboBox original)
        {
            throw new NotImplementedException();
        }

        public override Image GetImage(string id, Image original)
        {
            throw new NotImplementedException();
        }
    }
}
