using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class PreDeleteSpriteEventArgs
	{
		private bool _allowDelete = true;
		private int _spriteNumber;

		public PreDeleteSpriteEventArgs(int spriteNumber)
		{
			_spriteNumber = spriteNumber;
		}

		public bool AllowDelete
		{
			get { return _allowDelete; }
			set { _allowDelete = value; }
		}

		public int SpriteNumber
		{
			get { return _spriteNumber; }
		}
	}
}
