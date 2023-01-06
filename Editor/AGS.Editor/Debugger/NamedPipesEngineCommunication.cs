using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using System.Xml;

namespace AGS.Editor
{
    public class NamedPipesEngineCommunication : IEngineCommunication
    {
        public event MessageReceivedHandler MessageReceived;

        private NamedPipesServer _readServer;
        private NamedPipesServer _writeServer;
        private volatile bool _expectingAck = false;
        private bool _shuttingDown = false;
        private bool _abortSendMessage = false;
        private Thread _messageSender;
        private Queue _messageQueue = Queue.Synchronized(new Queue());
        private string _instanceIdentifier;
        private const string _separator_string = "<?xml version=\"1.0\"";
        private string[] _separator = new[] { _separator_string };

        public NamedPipesEngineCommunication()
        {
            _instanceIdentifier = System.Diagnostics.Process.GetCurrentProcess().Id.ToString();
            _readServer = new NamedPipesServer(@"\\.\pipe\AGSEditorDebuggerGameToEd" + _instanceIdentifier.ToString(), NamedPipeType.Read);
            _writeServer = new NamedPipesServer(@"\\.\pipe\AGSEditorDebuggerEdToGame" + _instanceIdentifier.ToString(), NamedPipeType.Write);
            _readServer.MessageReceived += new NamedPipesServer.MessageReceivedHandler(_server_MessageReceived);
            _readServer.Start();
            _writeServer.Start();

            _messageSender = new Thread(new ThreadStart(MessageSenderThread));
            _messageSender.Name = "MessageSender thread";
            _messageSender.Start();
        }

        public string InstanceIdentifier
        {
            get { return _instanceIdentifier; }
        }

        private void MessageSenderThread()
        {
            while (!_shuttingDown)
            {
                if (_messageQueue.Count > 0)
                {
                    if (MessageReceived != null)
                    {
                        string received_message = (string)_messageQueue.Dequeue();

                        // if more than one <?xml tag is on the string, it won't work. We now split using `<? xml version =\"1.0\"` as a separator string
                        // if somehow the message content contains this, it will crash, but lets hope this is not a problem.
                        string[] receivedXmls = received_message.Split(_separator, StringSplitOptions.RemoveEmptyEntries);
                        foreach (string receivedXml in receivedXmls)
                        {
                            if (string.IsNullOrEmpty(receivedXml)) continue;

                            XmlDocument doc = new XmlDocument();
                            string xmlString = _separator_string + receivedXml;

                            try
                            {
                                doc.LoadXml(xmlString);
                            }
                            catch
                            {
                                continue;
                            }
                            MessageReceived(doc);
                        }
                    }
                }
                else
                {
                    Thread.Sleep(10);
                }
            }
        }

        private void _server_MessageReceived(NamedPipesServer.Client client, string message)
        {
            if (message == "MSGACK")
            {
                _expectingAck = false;
                return;
            }

            // We use a separate thread to send the messages to avoid deadlocks
            // waiting to get the MSGACK
            _messageQueue.Enqueue(message);
        }

        public bool SupportedOnCurrentSystem
        {
            get { return true; }
        }

        public void ResetWithCurrentPath()
        {
        }

        public void SendMessage(string data)
        {
            while (_writeServer.ClientCount == 0)
            {
                if (_abortSendMessage)
                {
                    return;
                }

                System.Threading.Thread.Sleep(10);
            }

            _expectingAck = true;
            _writeServer.SendMessage(data);

            // Wait for the acknowledgement back that the engine has received
            while ((_expectingAck) && (!_abortSendMessage))
            {
                System.Threading.Thread.Sleep(10);
            }
        }

        public void ClientHasExited()
        {
            _abortSendMessage = true;
            _writeServer.RemoveAllClients();
            _readServer.RemoveAllClients();
        }

        public void NewClient()
        {
            _abortSendMessage = false;
            _writeServer.RemoveAllClients();
            _readServer.RemoveAllClients();
        }

        public void Dispose()
        {
            _shuttingDown = true;
            _readServer.Stop();
            _writeServer.Stop();
        }
    }
}
