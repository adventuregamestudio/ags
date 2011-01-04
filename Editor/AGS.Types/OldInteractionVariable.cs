using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class OldInteractionVariable
	{
		public const int LOCAL_VARIABLE_INDEX_OFFSET = 10000;

		public string Name;
		public int Value;

		public OldInteractionVariable(string name, int value)
		{
			Name = name;
			Value = value;
		}

		public string ScriptName
		{
			get
			{
				string scriptName = "IntVar_" + Name;
				for (int i = 0; i < scriptName.Length; i++)
				{
					if (!Char.IsLetterOrDigit(scriptName[i]))
					{
						scriptName = scriptName.Replace(scriptName[i], '_');
					}
				}
				return scriptName;
			}
		}
	}
}
