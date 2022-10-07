using System;
using System.Drawing;
using System.Linq.Expressions;
using System.Reflection;
using System.Windows.Forms;
using static System.Windows.Forms.Control;

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

        public abstract bool Has(string id);

        public bool Has(params string[] id)
        {
            bool found = false;
            foreach (string currentId in id)
            {
                if (Has(currentId)) found = true;
            }
            return found;
        }

        public abstract Color GetColor(string id);

        public Color GetColor(params string[] id)
        {
            foreach (string currentId in id)
            {
                if (Has(currentId))
                    return GetColor(currentId);
            }
            throw new ArgumentNullException();
        }
       
        public void SetColor(string id, Action<Color> fn)
        {
            try
            {
                if (Has(id))
                    fn(GetColor(id));
            }
            catch { }
        }

        public void SetColor(string[] id, Action<Color> fn)
        {
            foreach (string currentId in id)
            {
                try
                {
                    if (Has(currentId))
                    {
                        fn(GetColor(currentId));
                        return;
                    }
                }
                catch { }
            }
        }

        public abstract int GetInt(string id);

        public int GetInt(params string[] id)
        {
            foreach (string currentId in id)
            {
                if (Has(currentId))
                    return GetInt(currentId);
            }
            throw new ArgumentNullException();
        }

        public void SetInt(string id, Action<int> fn)
        {
            try
            {
                if (Has(id))
                    fn(GetInt(id));
            }
            catch { }
        }

        public void SetInt(string[] id, Action<int> fn)
        {
            foreach (string currentId in id)
            {
                try
                {
                    if (Has(currentId))
                    {
                        fn(GetInt(currentId));
                        return;
                    }
                }
                catch { }
            }
        }

        public abstract bool GetBool(string id);

        public bool GetBool(params string[] id)
        {
            foreach (string currentId in id)
            {
                if (Has(currentId))
                    return GetBool(currentId);
            }
            throw new ArgumentNullException();
        }

        public void SetBool(string id, Action<bool> fn)
        {
            try
            {
                if (Has(id))
                    fn(GetBool(id));
            }
            catch { }
        }

        public void SetBool(string[] id, Action<bool> fn)
        {
            foreach (string currentId in id)
            {
                try
                {
                    if (Has(currentId))
                    {
                        fn(GetBool(currentId));
                        return;
                    }
                }
                catch { }
            }
        }

        public abstract ToolStripRenderer GetMainMenuRenderer(string id);

        public abstract ToolStripRenderer GetToolStripRenderer(string id);

        public void GetToolStripRenderer(Func<ToolStripRenderer, ToolStripRenderer> tsr, params string[] id)
        {
            foreach (string currentId in id)
            {
                try
                {
                    tsr(GetToolStripRenderer(currentId));
                }
                catch { }
            }
        }

        public abstract ComboBox GetComboBox(string id, ComboBox original);

        public abstract Image GetImage(string id, Image original);

        public override string ToString() => Name;

        public void ControlHelper(Control control, string path)
        {
            SetColor(new string[] { path + "/background", "global/background" }, c => control.BackColor = c);
            SetColor(new string[] { path + "/foreground", "global/foreground" }, c => control.ForeColor = c);
        }

        public void GroupBoxHelper(GroupBox box, string path)
        {
            SetColor(new string[] { path + "/background", "global/box/background" }, c => box.BackColor = c);
            SetColor(new string[] { path + "/foreground", "global/box/foreground" }, c => box.ForeColor = c);
        }

        public void ButtonHelper(Button btn, string path)
        {
            SetColor(new string[] { path + "/background", "global/button/background" }, c => btn.BackColor = c);
            SetColor(new string[] { path + "/foreground", "global/button/foreground" }, c => btn.ForeColor = c);
            SetInt(new string[] { path + "/flat/style", "global/button/flat/style" }, i => btn.FlatStyle = (FlatStyle)i);
            SetInt(new string[] { path + "/flat/border/size", "global/button/flat/border/size" }, i => btn.FlatAppearance.BorderSize = i);
            SetColor(new string[] { path + "/flat/border/color", "global/button/flat/border/color" }, c => btn.FlatAppearance.BorderColor = c);
        }

        public void ListViewHelper(ListView lvw, string path)
        {
            SetColor(new string[] { path + "/background", "global/listview/background" }, c => lvw.BackColor = c);
            SetColor(new string[] { path + "/foreground", "global/listview/foreground" }, c => lvw.ForeColor = c);
            SetBool(new string[] { path + "/owner-draw", "global/listview/owner-draw" }, c => lvw.OwnerDraw = c);
            SetBool(new string[] { path + "/grid-lines", "global/listview/grid-lines" }, c => lvw.GridLines = c);

            if (Has(path + "/last-column-width", "global/listview/last-column-width"))
            {
                int lastColumnWidth = GetInt(path + "/last-column-width", "global/listview/last-column-width");
                lvw.Layout += (s, a) =>
                {
                    lvw.Columns[lvw.Columns.Count - 1].Width = lastColumnWidth;
                };
            }
            if (Has(path + "/draw-item", "global/listview/draw-item"))
            {
                bool drawItem = GetBool(path + "/draw-item", "global/listview/draw-item");
                lvw.DrawItem += (s, a) => a.DrawDefault = drawItem;
            }
            if (Has(path + "/draw-sub-item", "global/listview/draw-sub-item"))
            {
                bool drawSubItem = GetBool(path + "/draw-sub-item", "global/listview/draw-sub-item");
                lvw.DrawSubItem += (s, a) => a.DrawDefault = drawSubItem;
            }
            if (Has(path + "/column-header", "global/listview/column-header"))
            {
                Color background = GetColor(path + "/column-header/background", "global/listview/column-header/background");
                Color foreground = GetColor(path + "/column-header/foreground", "global/listview/column-header/foreground");
                Color border = GetColor(path + "/column-header/border", "global/listview/column-header/border");
                lvw.DrawColumnHeader += (s, a) =>
                {
                    a.Graphics.FillRectangle(new SolidBrush(background), a.Bounds);
                    a.Graphics.DrawString(a.Header.Text, lvw.Font, new SolidBrush(foreground), a.Bounds.X + 5, a.Bounds.Y + a.Bounds.Size.Height / 5);
                    a.Graphics.DrawRectangle(new Pen(new SolidBrush(border)), a.Bounds.X - 1, a.Bounds.Y - 1, a.Bounds.Size.Width, a.Bounds.Size.Height);
                };
            }
        }

        public bool ComboBoxHelper(ControlCollection ctrl, ref ComboBox cb, string path)
        {
            string cbTheme = null;
            if (Has(path)) cbTheme = path;
            else if (Has("global/combobox")) cbTheme = "global/combobox";

            if (cbTheme == null) return false;

            ctrl.Remove(cb);
            cb = GetComboBox(cbTheme, cb);
            ctrl.Add(cb);
            return true;

        }

        public void PropertyGridHelper(PropertyGrid grid, string path)
        {
            SetColor(new string[] { path + "/background", "global/propertygrid/background" }, c => grid.BackColor = c);
            SetColor(new string[] { path + "/view/background", "global/propertygrid/view/background" }, c => grid.ViewBackColor = c);
            SetColor(new string[] { path + "/view/foreground", "global/propertygrid/view/foreground" }, c => grid.ViewForeColor = c);
            SetColor(new string[] { path + "/view/border", "global/propertygrid/view/border" }, c => grid.ViewBorderColor = c);
            SetColor(new string[] { path + "/line", "global/propertygrid/line" }, c => grid.LineColor = c);
            SetColor(new string[] { path + "/category", path + "/category-fore", "global/propertygrid/category" }, c => grid.CategoryForeColor = c);
            SetColor(new string[] { path + "/splitter", "global/propertygrid/splitter" }, c => grid.CategorySplitterColor = c);
            SetColor(new string[] { path + "/help/background", "global/propertygrid/help/background" }, c => grid.HelpBackColor = c);
            SetColor(new string[] { path + "/help/foreground", "global/propertygrid/help/foreground" }, c => grid.HelpForeColor = c);
            SetColor(new string[] { path + "/help/border", "global/propertygrid/help/border" }, c => grid.HelpBorderColor = c);

            if (grid is CustomPropertyGrid)
            {
                if (Has("tool-bar"))
                {
                    ((CustomPropertyGrid)grid).SetRenderer(GetToolStripRenderer("tool-bar"));
                }
            }
  
        }

        public void TextBoxHelper(TextBox textbox, string path)
        {
            SetColor(new string[] { path + "/background", "global/textbox/background" }, c => textbox.BackColor = c);
            SetColor(new string[] { path + "/foreground", "global/textbox/foreground" }, c => textbox.ForeColor = c);
            SetInt(new string[] { path + "/border-style", "global/textbox/border-style" }, i => textbox.BorderStyle = (BorderStyle)i);
        }

        private PropertyInfo getPropertyFromPath(object instance, string path)
        {
            var parts = path.Split('.');
            Type t = instance.GetType();
            foreach (var prop in parts)
            {
                PropertyInfo propInfo = t.GetProperty(prop);
                if (propInfo != null)
                {
                    return propInfo;
                }
                else throw new ArgumentException("Property path invalid: "+ path);
            }
            return null; // will never reach here
        }
    }
}
