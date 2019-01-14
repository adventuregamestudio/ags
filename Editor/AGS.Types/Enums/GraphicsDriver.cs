using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
	[DeserializeConvertValueAttribute("DX5", "Software")]
	public enum GraphicsDriver
	{
		[Description("Software driver")]
		Software,
		[Description("Direct3D 9")]
		D3D9,
		[Description("OpenGL")]
		OpenGL
	}
}
