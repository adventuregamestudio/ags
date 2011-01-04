using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class VoiceActorScriptProcessor : SpeechOnlyProcessor
	{
		private Dictionary<int, Dictionary<string, string>> _linesByCharacter;
		private List<GameTextLine> _linesInOrder;

		public Dictionary<int, Dictionary<string, string>> LinesByCharacter
		{
			get { return _linesByCharacter; }
		}

		public List<GameTextLine> LinesInOrder
		{
			get { return _linesInOrder; }
		}

		public VoiceActorScriptProcessor(Game game, CompileMessages errors,
            Dictionary<string, FunctionCallType> speechableFunctionCalls) :
            base(game, errors, false, false, speechableFunctionCalls)
        {
			_linesByCharacter = new Dictionary<int, Dictionary<string, string>>();
			_linesInOrder = new List<GameTextLine>();
		}

		protected override string CreateSpeechLine(int speakingCharacter, string text)
		{
			if (text.TrimStart().StartsWith("&"))
			{
				if (speakingCharacter == -1)
				{
					speakingCharacter = Character.NARRATOR_CHARACTER_ID;
				}

				if (!_linesByCharacter.ContainsKey(speakingCharacter))
				{
					_linesByCharacter.Add(speakingCharacter, new Dictionary<string, string>());
				}
				if (!_linesByCharacter[speakingCharacter].ContainsKey(text))
				{
					_linesByCharacter[speakingCharacter].Add(text, text);
				}

				_linesInOrder.Add(new GameTextLine(speakingCharacter, text));
			}
			return text;
		}

	}
}
