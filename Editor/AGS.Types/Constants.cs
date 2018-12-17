using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public static class Constants
	{
		public const string SCRIPT_KEY_WORDS = "int char short long void return function string if else while struct import export readonly this enum bool false true managed null extends attribute static protected writeprotected float const noloopcheck new for break continue do switch case default";
		public const string AUTOCOMPLETE_ACCEPT_KEYS = "([,.=+-";
        public const string IMAGE_FILE_FILTER = "All supported images (*.bmp; *.gif; *.jpg; *.png; *.tif)|*.bmp;*.gif;*.jpg;*.png;*.tif|Windows bitmap files (*.bmp)|*.bmp|Compuserve Graphics Interchange (*.gif)|*.gif|JPEG (*.jpg)|*.jpg|Portable Network Graphics (*.png)|*.png|Tagged Image File (*.tif)|*.tif";
        public const string GAME_TEMPLATE_FILE_FILTER = "AGS game template files (*.agt)|*.agt";
        public const string ROOM_TEMPLATE_FILE_FILTER = "AGS room template files (*.art)|*.art";
	}
}
