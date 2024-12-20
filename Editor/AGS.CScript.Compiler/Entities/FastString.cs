using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    /// <summary>
    /// Optimised class to allow Substrings to be done without creating new
    /// copies of the string data in memory.
    /// TODO: implement Length, to be able to work with any range inside parent string.
    /// TODO: implement EndsWith.
    /// </summary>
    public class FastString
    {
        private string _data;
        private int _offset;
        private const StringComparison _useComparison = StringComparison.Ordinal;

        public FastString(string source)
        {
            _data = source ?? string.Empty;
            _offset = 0;
        }

        public FastString(string source, int offset)
        {
            if (source == null || offset < 0 || offset > source.Length)
            {
                throw new ArgumentNullException();
            }
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
            // FIXME: compare Char by Char instead? calling String.Substring here defeats the purpose of FastString!
            return (_data[_offset] == text[0]) && (_data.Substring(_offset).StartsWith(text, _useComparison));
        }

        public int IndexOf(string text, int offset)
        {
            return _data.IndexOf(text, _offset + offset, _useComparison) - _offset;
        }

        public int IndexOf(string text)
        {
            return _data.IndexOf(text, _offset, _useComparison) - _offset;
        }

        public int IndexOf(char text)
        {
            return _data.IndexOf(text, _offset) - _offset;
        }

        public int IndexOf(char text, int offset)
        {
            return _data.IndexOf(text, _offset + offset) - _offset;
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
