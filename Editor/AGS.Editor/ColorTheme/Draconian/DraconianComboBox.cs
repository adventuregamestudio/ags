using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class DraconianComboBox : ComboBox
    {

        private const int WM_PAINT = 0x000F;

        public DraconianComboBox(ComboBox comboBox)
        {
            this.Dock = comboBox.Dock;
            this.DropDownStyle = comboBox.DropDownStyle;
            this.FormattingEnabled = comboBox.FormattingEnabled;
            this.Location = new Point(comboBox.Location.X, comboBox.Location.Y);
            this.MaxDropDownItems = comboBox.MaxDropDownItems;
            this.Name = comboBox.Name;
            this.Size = new Size(comboBox.Width, comboBox.Height);
            this.TabIndex = comboBox.TabIndex;
            foreach (var item in comboBox.Items)
            {
                this.Items.Add(item);
            }
        }

        protected override void OnCreateControl()
        {
            base.OnCreateControl();

            this.DrawMode = DrawMode.OwnerDrawVariable;
            this.FlatStyle = FlatStyle.Flat;
            this.BackColor = Color.FromArgb(51, 51, 55);
            this.ForeColor = Color.FromArgb(241, 241, 241);
            this.DropDown += new EventHandler(this.ComboBox_Draconian_DropDownClosed);
            this.DropDownClosed += new EventHandler(this.ComboBox_Draconian_DropDown);
            this.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.ComboBox_Draconian_DrawItem);
        }

        protected override void WndProc(ref Message m)
        {
            base.WndProc(ref m);

            switch (m.Msg)
            {
                case DraconianComboBox.WM_PAINT:
                    Graphics graphics = Graphics.FromHwnd(Handle);
                    Rectangle rectBorder = new Rectangle(0, 0, Width, Height);
                    Rectangle rectButton = new Rectangle(this.Width - 19, 0, 19, this.Height);
                    Brush arrowBrush = new SolidBrush(SystemColors.ControlText);
                    graphics.SmoothingMode = SmoothingMode.HighQuality;

                    //Draw the border of the combo box
                    ControlPaint.DrawBorder(graphics, rectBorder, Color.FromArgb(51, 51, 55), ButtonBorderStyle.Solid);

                    //Draw button background and set arrow color.
                    if (this.DroppedDown)
                    {
                        graphics.FillRectangle(new SolidBrush(Color.FromArgb(0, 122, 204)), rectButton);
                        arrowBrush = new SolidBrush(Color.FromArgb(241, 241, 241));
                    }
                    else
                    {
                        graphics.FillRectangle(new SolidBrush(Color.FromArgb(51, 51, 55)), rectButton);
                        arrowBrush = new SolidBrush(Color.FromArgb(153, 153, 153));
                    }

                    //Create the path for the arrow
                    GraphicsPath path = new GraphicsPath();
                    PointF TopLeft = new PointF(this.Width - 13, (this.Height - 5) / 2);
                    PointF TopRight = new PointF(this.Width - 6, (this.Height - 5) / 2);
                    PointF Bottom = new PointF(this.Width - 9, (this.Height + 2) / 2);
                    path.AddLine(TopLeft, TopRight);
                    path.AddLine(TopRight, Bottom);

                    //Draw button arrow
                    graphics.FillPath(arrowBrush, path);
                    break;
                default:
                    break;
            }
        }

        private void ComboBox_Draconian_DrawItem(object sender, DrawItemEventArgs e)
        {
            if (e.Index >= 0)
            {
                ComboBox comboBox = sender as ComboBox;
                e.DrawBackground();
                if ((e.State & DrawItemState.Selected) == DrawItemState.Selected)
                    e.Graphics.FillRectangle(new SolidBrush(Color.FromArgb(63, 63, 70)), e.Bounds);
                else
                    e.Graphics.FillRectangle(new SolidBrush(comboBox.BackColor), e.Bounds);
                e.Graphics.DrawString(comboBox.Items[e.Index].ToString(), comboBox.Font, new SolidBrush(comboBox.ForeColor), e.Bounds);
                e.DrawFocusRectangle();
            }
        }

        private void ComboBox_Draconian_DropDown(object sender, EventArgs e)
        {
            ComboBox comboBox = sender as ComboBox;
            comboBox.BackColor = Color.FromArgb(51, 51, 55);
            comboBox.ForeColor = Color.FromArgb(241, 241, 241);
        }

        private void ComboBox_Draconian_DropDownClosed(object sender, EventArgs e)
        {
            ComboBox comboBox = sender as ComboBox;
            comboBox.BackColor = Color.FromArgb(51, 51, 55);
            comboBox.ForeColor = Color.FromArgb(241, 241, 241);
        }
    }
}