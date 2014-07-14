using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    /// <summary>
    /// Optimised class to allow Substrings to be done without creating new
    /// copies of the string data in memory
    /// </summary>
    public class FastString
    {
        private string _data;
        private int _offset;

        public FastString(string source)
        {
            _data = source;
            _offset = 0;
        }

        public FastString(string source, int offset)
        {
            _data = source;
            _offset = offset;
        }

        public static implicit operator FastString(string a)
        {
            return new FastString(a);
        }

        public char this[int index]
        {
            get { return _data[index + _offset]; }
        }

        public int Length
        {
            get { return _data.Length - _offset; }
        }

        public bool StartsWith(string text)
        {
            if (_offset >= _data.Length)
            {
                return false;
            }
            return (_data[_offset] == text[0]) && (_data.Substring(_offset).StartsWith(text));
        }

        public int IndexOf(string text, int offset)
        {
            return _data.IndexOf(text, _offset + offset) - _offset;
        }

        public int IndexOf(string text)
        {
            return _data.IndexOf(text, _offset) - _offset;
        }

        public int IndexOf(char text)
        {
            return _data.IndexOf(text, _offset) - _offset;
        }

        public FastString Substring(int offset)
        {
            return new FastString(_data, _offset + offset);
        }

        public string Substring(int offset, int length)
        {
            return _data.Substring(_offset + offset, length);
        }

        public override string ToString()
        {
            if (_offset > 0)
            {
                return _data.Substring(_offset);
            }
            return _data;
        }

        public FastString Trim()
        {
            return new FastString(this.ToString().Trim());
        }
    }

}
