using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using AGS.Types.Enums;

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
        private System.Threading.Thread _messageProcThread;
        private object _commLock = new object();
        private object _messageQueueLock = new object();
        private List<string> _messageQueue = new List<string>();

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
        /// Tells whether Debugger is in a active working state.
        /// </summary>
        public bool IsActive
        {
            get { return _debugState != DebugState.NotRunning; }
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

            if (newState == DebugState.NotRunning)
            {
                StopMessageThread();
            }
            else if (_messageProcThread == null)
            {
                StartMessageThread();
            }

            if (DebugStateChanged != null)
            {
                DebugStateChanged(_debugState);
            }
        }

        private void StopMessageThread()
        {
            ClearMessageQueue();
            if (_messageProcThread != null)
            {
                _messageProcThread.Join();
                _messageProcThread = null;
            }
        }

        private void StartMessageThread()
        {
            ClearMessageQueue();
            if (_messageProcThread == null)
            {
                _messageProcThread = new System.Threading.Thread(new System.Threading.ThreadStart(MessageProcThread));
                _messageProcThread.Name = "DebugController message thread";
                _messageProcThread.Start();
            }
        }

        private void ClearMessageQueue()
        {
            lock (_messageQueueLock)
            {
                _messageQueue.Clear();
            }
        }

        /// <summary>
        /// Put a message into queue.
        /// This method is suitable when you do not require an instant response.
        /// </summary>
        private void QueueMessage(string message)
        {
            lock (_messageQueueLock)
            {
                _messageQueue.Add(message);
            }
        }

        /// <summary>
        /// Send message directly, surpassing the queue.
        /// This method is suitable when you must have a synchronous reaction from the engine.
        /// </summary>
        private void SendMessageDirect(string message)
        {
            lock (_commLock)
            {
                _communicator.SendMessage(message);
            }
        }

        private void MessageProcThread()
        {
            while (IsActive)
            {
                // Make a local copy of messages to send
                List<string> newMessagesToSend = null;

                lock (_messageQueueLock)
                {
                    if (_messageQueue.Count > 0)
                        newMessagesToSend = new List<string>(_messageQueue);
                    _messageQueue.Clear();
                }

                // Process messages
                if (newMessagesToSend != null)
                {
                    foreach (var msg in newMessagesToSend)
                    {
                        lock (_commLock)
                        {
                            _communicator.SendMessage(msg);
                        }
                    }
                }

                // Sleep
                System.Threading.Thread.Sleep(10);
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
            else if (command == "LOG")
            {
                XmlNode logTextNode = doc.DocumentElement.SelectSingleNode("Text");
                XmlNode logGroupIDNode = doc.DocumentElement.SelectSingleNode("GroupID");
                XmlNode logMTIDNode = doc.DocumentElement.SelectSingleNode("MTID");
                LogGroup group;
                LogLevel level;
                try
                {
                    group = (LogGroup)Convert.ToInt32(logGroupIDNode.InnerText);
                    level = (LogLevel)Convert.ToInt32(logMTIDNode.InnerText);
                }
                catch
                {
                    return;
                }
                LogMessage(logTextNode.InnerText, group, level);
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
            SendMessageDirect("<Engine Command=\"START\" EditorWindow=\"" + editorHwnd + "\" />");
            ChangeDebugState(DebugState.Running);

            foreach (Script script in game.GetAllGameAndLoadedRoomScripts())
            {
                foreach (int line in script.BreakpointedLines)
                {
                    SetBreakpoint(script, line);
                }
            }

            SendMessageDirect("<Engine Command=\"READY\" EditorWindow=\"" + editorHwnd + "\" />");
        }

        public void LogMessage(string message, LogGroup group, LogLevel level)
        {
            Factory.GUIController.PrintEngineLog(message, group, level);
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
            SendMessageDirect("<Engine Command=\"SETBREAK $" + script.FileName + "$" + lineNumber + "$\"></Engine>");
        }

        private void UnsetBreakpoint(Script script, int lineNumber)
        {
            SendMessageDirect("<Engine Command=\"DELBREAK $" + script.FileName + "$" + lineNumber + "$\"></Engine>");
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
                if (Utilities.IsMonoRunning())
                {
                    //Is there any way of doing this in mono? I couldn't find any.
                    //I guess the user will have to bring the game back to focus by himself...                    
                }
                else
                {
                    NativeProxy.SetForegroundWindow(_engineWindowHandle);
                }
            }
            SendMessageDirect("<Engine Command=\"" + command + "\" />");
        }

        public void Resume()
        {
            if (IsActive)
            {
                SendCommandAndSwitchWindows("RESUME");
            }
        }

        public void StepInto()
        {
            if (IsActive)
            {
                SendCommandAndSwitchWindows("STEP");
            }
        }

        public void PauseExecution()
        {
            if (IsActive)
            {
                // the STEP command will break out at the next line
                SendCommandAndSwitchWindows("STEP");
            }
        }

        public void StopDebugging()
        {
            if (IsActive)
            {
                SendCommandAndSwitchWindows("EXIT");
            }
        }

        public void EditorShutdown()
        {
            StopMessageThread();
            _communicator.Dispose();
        }
    }
}
