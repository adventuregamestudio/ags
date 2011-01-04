using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
	public class VoiceActorScriptGenerator : BaseTextProcess
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

		public CompileMessages CreateVoiceActingScript(Game game)
		{
			CompileMessages errors = new CompileMessages();

			VoiceActorScriptProcessor processor = new VoiceActorScriptProcessor(game, errors, GetFunctionCallsToProcessForSpeech(true));

			TextProcessingHelper.ProcessAllGameText(processor, game, errors);

			_linesByCharacter = processor.LinesByCharacter;
			_linesInOrder = processor.LinesInOrder;

			return errors;
		}
	}
}
