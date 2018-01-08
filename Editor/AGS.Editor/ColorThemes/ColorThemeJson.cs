using System;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using Newtonsoft.Json.Linq;

namespace AGS.Editor
{
    public class ColorThemeJson : ColorTheme
    {
        private readonly JObject _json;

        public ColorThemeJson(string name, string dir) : base(name)
        {
            _json = JObject.Parse(File.ReadAllText(dir));
        }

        public override Color GetColor(string id)
        {
            return DoTransform(id, t => Color.FromArgb((int)t["a"], (int)t["r"], (int)t["g"], (int)t["b"]));
        }

        public override int GetInt(string id)
        {
            return DoTransform(id, t => (int)t);
        }

        public override bool GetBool(string id)
        {
            return DoTransform(id, t => (bool)t);
        }

        public override ToolStripRenderer GetMainMenuRenderer(string id)
        {
            return DoTransform(id, t => new ToolStripProfessionalRenderer(new MainMenuColorTable(this, t.Path)));
        }

        public override ToolStripRenderer GetToolStripRenderer(string id)
        {
            return DoTransform(id, t => new ToolStripProfessionalRenderer(new ToolStripColorTable(this, t.Path)));
        }

        public override ComboBox GetComboBox(string id, ComboBox original)
        {
            return DoTransform(id, t => new ComboBoxCustom(this, t.Path, original));
        }

        public override Image GetImage(string id, Image original)
        {
            return DoTransform(id, t =>
            {
                Bitmap b = new Bitmap(original, original.Size);

                for (int y = 0; y < original.Height; y++)
                {
                    for (int x = 0; x < original.Width; x++)
                    {
                        if (b.GetPixel(x, y).A > 0)
                        {
                            b.SetPixel(x, y, Color.FromArgb((int)t["a"], (int)t["r"], (int)t["g"], (int)t["b"]));
                        }
                    }
                }

                return b;
            });
        }

        private T DoTransform<T>(string id, Func<JToken, T> transform) => transform(GetJToken(id, _json));

        private static JToken GetJToken(string id, JObject json)
        {
            string[] tokens = id.Replace('.', '/').Split('/');
            JToken token = json[tokens[0]];
            tokens.Skip(1).ToList().ForEach(t => token = token[t]);
            return token;
        }
    }
}
