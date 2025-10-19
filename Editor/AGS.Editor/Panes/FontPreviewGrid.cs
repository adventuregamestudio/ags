using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class FontPreviewGrid : BufferedPanel
    {
        private AGS.Types.Font _font;
        private Native.FontMetrics _fontMetrics;
        // Valid character codes for the current font
        private int[] _charCodes;
        // Character code to preview index lookup table
        private Dictionary<int, int> _charcodeToCellIndex;
        private int _lastANSICharCodeIndex = -1;
        private float _scaling = 1.0f;
        private bool _ansiMode = false;
        private bool _hideMissingChars = true;
        private int _selectedChar = -1;
        private bool _displayCodes = false;

        public FontPreviewGrid()
        {
            // Assign some default preview grid properties
            _codeCue.Font = Font;
            Size size = TextRenderer.MeasureText("0000", _codeCue.Font);
            _codeCue.Width = size.Width + 2;
            _codeCue.Height = size.Height + 2;

            PrecalculatePreviewGrid();
        }

        /// <summary>
        /// Game font to display.
        /// </summary>
        [Browsable(false)]
        public AGS.Types.Font GameFont
        {
            get
            {
                return _font;
            }
            set
            {
                _font = value;
                if (DesignMode)
                    return;

                UpdateAndRepaint(true);
            }
        }

        public float Scaling
        {
            get
            {
                return _scaling;
            }
            set
            {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException();

                _scaling = value;
                if (!DesignMode)
                {
                    UpdateAndRepaint(false);
                }
            }
        }

        public bool ANSIMode
        {
            get { return _ansiMode; }
            set
            {
                _ansiMode = value;
                if (!DesignMode)
                {
                    UpdateAndRepaint(false);
                }
            }
        }

        public bool HideMissingCharacters
        {
            get { return _hideMissingChars; }
            set
            {
                _hideMissingChars = value;
                if (!DesignMode)
                {
                    UpdateAndRepaint(false);
                }
            }
        }

        [Browsable(false)]
        public int SelectedCharCode
        {
            get
            {
                return _selectedChar;
            }
            set
            {
                if (!DesignMode)
                {
                    // Any out of range value (negative too) will act as "no selection"
                    SelectAndScrollToCharacter(value);
                }
            }
        }

        /// <summary>
        /// Tells if the character codes should be displayed in preview
        /// </summary>
        public bool DisplayCodes
        {
            get
            {
                return _displayCodes;
            }
            set
            {
                _displayCodes = value;
                if (!DesignMode)
                {
                    UpdateAndRepaint(false);
                }
            }
        }

        [Browsable(false)]
        public bool GameFontValid
        {
            get { return !DesignMode && _font != null && _font.FontFile != null; }
        }

        public class CharacterSelectedEventArgs : EventArgs
        {
            public int CharacterCode { get; private set; }

            public CharacterSelectedEventArgs(int characterCode)
            {
                CharacterCode = characterCode;
            }
        }

        public delegate void CharacterSelectedHandler(object sender, CharacterSelectedEventArgs args);
        public event CharacterSelectedHandler CharacterSelected;

        /// <summary>
        /// Parameters of the character code cue, optionally drawn below characters.
        /// </summary>
        struct CodeCue
        {
            public int Width;
            public int Height;
            public System.Drawing.Font Font;
        };

        private CodeCue _codeCue;

        /// <summary>
        /// Measurements of the character grid itself.
        /// Note that all of the grid sizes are defined in "local" grid coordinates,
        /// which must be scaled by the chosen factor when translating to the control's
        /// client area.
        /// </summary>
        struct PreviewGrid
        {
            public int GridWidth; // visible grid width
            public int GridHeight; // visible grid height
            public int CellWidth;
            public int CellHeight;
            public int CellSpaceX; // horizontal spacing between character cells
            public int CellSpaceY; // vertical spacing between character cells
            public int CharsPerRow; // how many characters fit a single row

            public int FullHeight; // a virtual full preview height
            public int MaxScrollY; // maximal value of a virtual scroll
            public int ScrollY; // a virtual scroll y
        };

        private PreviewGrid _grid;

        public void UpdateAndRepaint(bool reloadFont)
        {
            if (GameFontValid)
            {
                _fontMetrics = Factory.NativeProxy.GetFontMetrics(_font.ID);
                if (reloadFont)
                {
                    _charcodeToCellIndex = new Dictionary<int, int>();
                    _charCodes = Factory.NativeProxy.GetFontValidCharacters(_font.ID);
                    for (int i = 0; i < _charCodes.Length; ++i)
                    {
                        _charcodeToCellIndex.Add(_charCodes[i], i);
                        if (_charCodes[i] < 256)
                            _lastANSICharCodeIndex = i;
                    }
                }
            }
            else
            {
                _fontMetrics = Native.FontMetrics.Empty;
                _charCodes = null;
                _charcodeToCellIndex = null;
                _lastANSICharCodeIndex = -1;
            }

            PrecalculatePreviewGrid();
            _grid.ScrollY = MathExtra.Clamp(_grid.ScrollY, 0, _grid.MaxScrollY);

            Invalidate();
        }

        private void PrecalculatePreviewGrid()
        {
            if (!GameFontValid)
                return;

            if (ClientSize.Width <= 0 || ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int width = ClientSize.Width;
            int height = ClientSize.Height;
            // When reading BBox be aware that it may be "shifted" to the negative and positive side,
            // but the drawing origin is always at 0,0.
            Rectangle bbox = _fontMetrics.CharBBox;
            int cell_w = Math.Max(10, Math.Max(0, bbox.Left) + bbox.Width);
            int cell_h = Math.Max(10, Math.Max(0, bbox.Top) + bbox.Height);
            // We want cell spacing to include a place for printing character codes;
            // remember that the cue sizes are in unscaled coords, so pre-unscale them here
            int cell_space_x = Math.Max(cell_w / 4, (int)(_codeCue.Width / _scaling) + 1 - cell_w);
            int cell_space_y = Math.Max(cell_h / 4, (int)(_codeCue.Height / _scaling) + 1);
            int grid_width = (int)(width / _scaling);
            int grid_height = (int)(height / _scaling);

            // Precalculate full height
            // Chars per row do not include partially visible ones, unless there's just 1 column
            int chars_per_row = Math.Max(1, (grid_width - cell_space_x) / (cell_w + cell_space_x));
            int char_count = 0;
            if (HideMissingCharacters)
            {
                if (_charCodes.Length > 0)
                {
                    char_count = ANSIMode ?
                        _lastANSICharCodeIndex + 1 :
                        _charCodes.Length;
                }
            }
            else
            {
                int first_char = 0;
                int last_char = _fontMetrics.LastCharCode;
                if (ANSIMode)
                    last_char = Math.Min(last_char, 255);
                char_count = (last_char - first_char + 1);
            }

            int full_height = (char_count / chars_per_row + 1) * (cell_h + cell_space_y)
                 + cell_space_y;

            _grid.GridWidth = grid_width;
            _grid.GridHeight = grid_height;
            _grid.CellWidth = cell_w;
            _grid.CellHeight = cell_h;
            _grid.CellSpaceX = cell_space_x;
            _grid.CellSpaceY = cell_space_y;
            _grid.CharsPerRow = chars_per_row;
            _grid.FullHeight = full_height;
            _grid.MaxScrollY = Math.Max(0, full_height - grid_height);

            // Set virtual preview height (the virtual size of the contents within the font preview)
            AutoScrollMinSize = new Size(0, (int)(full_height * _scaling));
        }

        private Point ControlToGrid(int x, int y)
        {
            return new Point(
                (int)(x / _scaling),
                (int)(y / _scaling) + _grid.ScrollY);
        }

        private Point ControlToGrid(Point pt)
        {
            return ControlToGrid(pt.X, pt.Y);
        }

        private Point GridToControl(int x, int y)
        {
            return new Point(
                (int)((x) * _scaling),
                (int)((y - _grid.ScrollY) * _scaling));
        }

        private Point GridToControl(Point pt)
        {
            return GridToControl(pt.X, pt.Y);
        }

        private Rectangle GridToControl(Rectangle rect)
        {
            return new Rectangle(
                GridToControl(rect.X, rect.Y),
                new Size((int)(rect.Width * _scaling), (int)(rect.Height * _scaling))
                );
        }

        private bool CharCodeToColRow(int code, out int col, out int row)
        {
            if (code < 0 || _grid.CharsPerRow == 0)
            {
                col = -1;
                row = -1;
                return false;
            }

            int cell;
            if (HideMissingCharacters)
            {
                if (!_charcodeToCellIndex.TryGetValue(code, out cell))
                {
                    col = -1;
                    row = -1;
                    return false;
                }
            }
            else
            {
                cell = code;
            }

            col = cell % _grid.CharsPerRow;
            row = cell / _grid.CharsPerRow;
            return true;
        }

        private Point CharCodePosition(int code)
        {
            if (code < 0 || _grid.CharsPerRow == 0)
            {
                return new Point(-1, -1);
            }

            int col, row;
            if (!CharCodeToColRow(code, out col, out row))
            {
                return new Point(-1, -1);
            }

            return CellPosition(col, row);
        }

        private bool PointToColRow(Point pt, out int col, out int row)
        {
            if (pt.X < 0 || pt.Y < 0 || _grid.CharsPerRow == 0)
            {
                col = -1;
                row = -1;
                return false;
            }

            col = Math.Min((pt.X - _grid.CellSpaceX) / (_grid.CellWidth + _grid.CellSpaceX), _grid.CharsPerRow - 1);
            row = (pt.Y - _grid.CellSpaceY) / (_grid.CellHeight + _grid.CellSpaceY);
            return true;
        }

        /// <summary>
        /// Returns left-top position of a cell in grid's coordinates
        /// </summary>
        private Point CellPosition(int col, int row)
        {
            return new Point(
                col * (_grid.CellWidth + _grid.CellSpaceX) + _grid.CellSpaceX,
                row * (_grid.CellHeight + _grid.CellSpaceY) + _grid.CellSpaceY);
        }

        private int CharCodeAtCell(int col, int row)
        {
            int cell = row * _grid.CharsPerRow + col;
            if (HideMissingCharacters)
            {
                return (cell < _charCodes.Length) ? _charCodes[cell] : -1;
            }
            else
            {
                return cell;
            }
        }

        private void SelectAndScrollToCharacter(int code)
        {
            if (!GameFontValid ||
                code < 0 || code > _fontMetrics.LastCharCode)
            {
                _selectedChar = -1;
                Invalidate(); // might need to erase selection box
                return;
            }

            if (_grid.FullHeight == 0)
            {
                PrecalculatePreviewGrid();
            }

            // Try to calculate the necessary scroll Y which lets us see the wanted character
            var pos = CharCodePosition(code);
            if (pos.X < 0 || pos.Y < 0)
            {
                _selectedChar = -1;
                Invalidate(); // might need to erase selection box
                return;
            }

            // If y is NOT within the visible range, then do the minimal necessary scroll
            bool do_scroll = false;
            if (pos.Y < _grid.ScrollY)
            {
                do_scroll = true;
            }
            else
            {
                if ((pos.Y + _grid.CellHeight + _grid.CellSpaceY) > (_grid.ScrollY + _grid.GridHeight))
                {
                    pos.Y = (pos.Y + _grid.CellHeight + _grid.CellSpaceY) - _grid.GridHeight;
                    do_scroll = true;
                }
            }

            _selectedChar = code;
            if (do_scroll)
            {
                _grid.ScrollY = MathExtra.Clamp(pos.Y, 0, _grid.MaxScrollY);
                AutoScrollPosition = new Point(0, (int)(pos.Y * _scaling));
            }
            CharacterSelected?.Invoke(this, new CharacterSelectedEventArgs(_selectedChar));
            Invalidate();
        }

        private void SelectCharacterAt(Point pt)
        {
            if (!GameFontValid)
                return;

            if (_grid.GridHeight == 0)
            {
                PrecalculatePreviewGrid();
            }

            Point gridPt = ControlToGrid(pt);
            if (gridPt.X < 0 || gridPt.Y < 0 || gridPt.X >= _grid.GridWidth || gridPt.Y >= _grid.FullHeight)
                return;

            int col, row;
            PointToColRow(gridPt, out col, out row);
            SelectAndScrollToCharacter(CharCodeAtCell(col, row));
        }

        private void PaintFont(Graphics g)
        {
            if (DesignMode)
                return;

            if (ClientSize.Width <= 0 || ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            if (!GameFontValid)
            {
                g.Clear(Color.Black);
                return;
            }

            if (_grid.FullHeight == 0)
            {
                PrecalculatePreviewGrid();
            }

            int width = ClientSize.Width;
            int height = ClientSize.Height;
            int scroll_y = _grid.ScrollY;
            int firstVisibleRow = (scroll_y) / (_grid.CellHeight + _grid.CellSpaceY);
            int firstVisibleCell = firstVisibleRow * _grid.CharsPerRow;
            int lastVisibleRow = firstVisibleRow + (_grid.GridHeight / (_grid.CellHeight + _grid.CellSpaceY));
            int lastVisibleCell = lastVisibleRow * _grid.CharsPerRow + _grid.CharsPerRow;
            lastVisibleCell = Math.Min(lastVisibleCell, _charCodes.Length - 1);

            // Always clear the panel first
            g.Clear(Color.Black);
            bool hdcReleased = false;
            try
            {
                int firstRowDrawPos = _grid.CellSpaceY + firstVisibleRow * (_grid.CellHeight + _grid.CellSpaceY)
                    - scroll_y;
                Factory.NativeProxy.DrawFont(g.GetHdc(), _font.ID, ANSIMode, HideMissingCharacters,
                    0, 0, _grid.CellSpaceX, firstRowDrawPos,
                    _grid.CellWidth, _grid.CellHeight, _grid.CellSpaceX, _grid.CellSpaceY,
                    _grid.CharsPerRow, lastVisibleRow - firstVisibleRow + 1, firstVisibleCell,
                    _scaling);
                g.ReleaseHdc();
                hdcReleased = true;
            }
            catch (Exception)
            {
            }
            finally
            {
                if (!hdcReleased)
                    g.ReleaseHdc();
            }

            // Additional visual cues
            if (_selectedChar >= 0)
            {
                Point cellPos = CharCodePosition(_selectedChar);
                Rectangle pos = new Rectangle(cellPos,
                    new Size(
                        DisplayCodes ? Math.Max(_grid.CellWidth, (int)(_codeCue.Width / _scaling)) : _grid.CellWidth,
                        DisplayCodes ? (_grid.CellHeight + _grid.CellSpaceY) : _grid.CellHeight));
                pos = GridToControl(pos);
                g.DrawRectangle(Pens.Yellow, pos);
            }

            // Print char codes
            if (DisplayCodes)
            {
                int lastCharCode = _fontMetrics.LastCharCode;
                if (ANSIMode)
                    lastCharCode = Math.Min(lastCharCode, 255);
                for (int cell = firstVisibleCell, row = firstVisibleRow; row <= lastVisibleRow && cell <= lastVisibleCell; ++row)
                {
                    for (int col = 0; col < _grid.CharsPerRow && cell <= lastVisibleCell; ++col, ++cell)
                    {
                        int code = HideMissingCharacters ? _charCodes[cell] : cell;
                        if (code > lastCharCode)
                            break;

                        // x,y and cell sizes are in scaled grid coordinates, but cue size is not scaled
                        Point cellLeftBottom = GridToControl(CellPosition(col, row + 1));
                        Rectangle pos = new Rectangle(cellLeftBottom.X, cellLeftBottom.Y - _codeCue.Height,
                            _codeCue.Width, _codeCue.Height);
                        g.FillRectangle(Brushes.LightGray, pos);
                        if (ANSIMode)
                            g.DrawString(code.ToString(), _codeCue.Font, Brushes.Black, pos.X + 1, pos.Y + 1);
                        else
                            g.DrawString(code.ToString("X4"), _codeCue.Font, Brushes.Black, pos.X + 1, pos.Y + 1);
                    }
                }
            }
        }

        protected override void OnSizeChanged(EventArgs e)
        {
            base.OnSizeChanged(e);
            PrecalculatePreviewGrid();
            _grid.ScrollY = MathExtra.Clamp(_grid.ScrollY, 0, _grid.MaxScrollY);
            Invalidate();
        }

        protected override void OnScroll(ScrollEventArgs se)
        {
            base.OnScroll(se);
            _grid.ScrollY = MathExtra.Clamp((int)(-AutoScrollPosition.Y / _scaling), 0, _grid.MaxScrollY);
            Invalidate();
        }

        protected override void OnMouseClick(MouseEventArgs e)
        {
            base.OnMouseClick(e);
            Focus();
            SelectCharacterAt(e.Location);
        }

        protected override void OnMouseWheel(MouseEventArgs e)
        {
            // Call base first, will update the scroll pos after the wheel
            base.OnMouseWheel(e);
            _grid.ScrollY = MathExtra.Clamp((int)(-AutoScrollPosition.Y / _scaling), 0, _grid.MaxScrollY);
            Invalidate();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            PaintFont(e.Graphics);
        }
    }
}
