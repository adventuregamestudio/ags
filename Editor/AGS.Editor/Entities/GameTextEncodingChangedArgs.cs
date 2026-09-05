using AGS.CScript.Compiler;
using AGS.Types;
using System;
using System.Text;

namespace AGS.Editor
{
    public class GameTextEncodingChangedArgs
    {
        public GameTextEncodingChangedArgs(Game game, Encoding oldEnc, IWorkProgress progress, CompileMessages errors)
        {
            Game = game;
            OldEncoding = oldEnc;
            Progress = progress;
            Errors = errors;
        }

        public Game Game { get; private set; }
        public Encoding OldEncoding { get; private set; }
        public IWorkProgress Progress { get; private set; }
        public CompileMessages Errors { get; private set; }
    }
}
