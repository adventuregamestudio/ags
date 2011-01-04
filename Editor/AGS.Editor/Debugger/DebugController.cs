using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Editor
{
    public class DebugController
    {
        public delegate void DebugStateChangedHandler(DebugState newState);
        public event DebugStateChangedHandler DebugStateChanged;
        public delegate void BreakAtLocationHandler(DebugCallStack callStack);
        public event BreakAtLocationHandler BreakAtLocation;

        private DebugState _debugState = DebugState.NotRunning;
        private IEngineCommunication _communicator;
        private IntPtr _engineWindowHandle = IntPtr.Zero;

        public DebugController(IEngineCommunication communicator)
        {
            _communicator = communicator;
            _communicator.MessageReceived += new MessageReceivedHandler(_communicator_MessageReceived);
        }

		public bool CanUseDebugger
		{
			get { return _communicator.SupportedOnCurrentSystem; }
		}

        /// <summary>
        /// Allows more than one instance of the AGS Editor to run simulatenously
        /// </summary>
        public string InstanceIdentifier
        {
            get { return _communicator.InstanceIdentifier; }
        }

        private void ChangeDebugState(DebugState newState)
        {
            _debugState = newState;

            if (DebugStateChanged != null)
            {
                DebugStateChanged(_debugState);
            }
        }

        private void _communicator_MessageReceived(XmlDocument doc)
        {
            string command = doc.DocumentElement.Attributes["Command"].InnerText;

            _engineWindowHandle = new IntPtr(Convert.ToInt32(doc.DocumentElement.SelectSingleNode("EngineWindow").InnerText));

            if ((command == "BREAK") || (command == "ERROR"))
            {
                ChangeDebugState(DebugState.Paused);
                if (BreakAtLocation != null)
                {
                    string errorMessage = null;
                    XmlNode errorMsgNode = doc.DocumentElement.SelectSingleNode("ErrorMessage");
                    if (errorMsgNode != null)
                    {
                        errorMessage = errorMsgNode.InnerText;
                    }
                    string callStack = doc.DocumentElement.SelectSingleNode("ScriptState").InnerText;
                    BreakAtLocation(ParseCallStackIntoObjectForm(callStack, errorMessage));
                }
            }
            else if (command == "EXIT")
            {
				EngineHasExited();
            }
        }

        private DebugCallStack ParseCallStackIntoObjectForm(string callStackFromEngine, string errorMessage)
        {
            DebugCallStack result = new DebugCallStack(errorMessage);
            string[] callStackLines = callStackFromEngine.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string callStackEntry in callStackLines)
            {
                // There's a  (and more...)  if the call stack is too long, ignore it
                int firstSpeechMark = callStackEntry.IndexOf('"');
                if (firstSpeechMark >= 0)
                {
                    string callStack = callStackEntry.Substring(firstSpeechMark + 1);
                    string scriptName = callStack.Substring(0, callStack.IndexOf('"'));
                    string lineNumberText = callStack.Substring(callStack.IndexOf('"'));
                    lineNumberText = lineNumberText.Substring(lineNumberText.IndexOf("line ") + 5);
                    int i = 0;
                    while ((i < lineNumberText.Length) &&
                           (Char.IsDigit(lineNumberText, i)))
                    {
                        i++;
                    }
                    lineNumberText = lineNumberText.Substring(0, i);
                    result.AddLine(scriptName, Convert.ToInt32(lineNumberText));
                }
            }
            return result;
        }

        public void InitializeEngine(Game game, IntPtr editorHwnd)
        {
            _communicator.NewClient();
            _communicator.SendMessage("<Engine Command=\"START\" EditorWindow=\"" + editorHwnd + "\" />");
            ChangeDebugState(DebugState.Running);

            foreach (Script script in game.GetAllGameAndLoadedRoomScripts())
            {
                foreach (int line in script.BreakpointedLines)
                {
                    SetBreakpoint(script, line);
                }
            }

            _communicator.SendMessage("<Engine Command=\"READY\" EditorWindow=\"" + editorHwnd + "\" />");
        }

		public void EngineHasExited()
		{
            _communicator.ClientHasExited();
            ChangeDebugState(DebugState.NotRunning);
			ClearCurrentLineMarker();
		}

        public void AddedBreakpoint(Script script, int lineNumber)
        {
            if ((_debugState == DebugState.Paused) ||
                (_debugState == DebugState.Running))
            {
                SetBreakpoint(script, lineNumber);
            }
        }

        public void RemovedBreakpoint(Script script, int lineNumber)
        {
            if ((_debugState == DebugState.Paused) ||
                (_debugState == DebugState.Running))
            {
                UnsetBreakpoint(script, lineNumber);
            }
        }

        private void SetBreakpoint(Script script, int lineNumber)
        {
            _communicator.SendMessage("<Engine Command=\"SETBREAK $" + script.FileName + "$" + lineNumber + "$\"></Engine>");
        }

        private void UnsetBreakpoint(Script script, int lineNumber)
        {
            _communicator.SendMessage("<Engine Command=\"DELBREAK $" + script.FileName + "$" + lineNumber + "$\"></Engine>");
        }

        private void ClearCurrentLineMarker()
        {
            Factory.GUIController.ZoomToFile(string.Empty, 0, true, null);
        }

        private void SendCommandAndSwitchWindows(string command)
        {
            ClearCurrentLineMarker();

            ChangeDebugState(DebugState.Running);
            if (_engineWindowHandle != IntPtr.Zero)
            {
                NativeProxy.SetForegroundWindow(_engineWindowHandle);
            }
            _communicator.SendMessage("<Engine Command=\"" + command + "\" />");
        }

        public void Resume()
        {
            SendCommandAndSwitchWindows("RESUME");
        }

        public void StepInto()
        {
            SendCommandAndSwitchWindows("STEP");
        }

        public void PauseExecution()
        {
            // the STEP command will break out at the next line
            SendCommandAndSwitchWindows("STEP");
        }

        public void StopDebugging()
        {
            SendCommandAndSwitchWindows("EXIT");
        }

        public void EditorShutdown()
        {
            _communicator.Dispose();
        }
    }
}
