using System;
using System.Collections.Generic;
using System.Xml;

namespace AGS.Editor
{
    public delegate void MessageReceivedHandler(XmlDocument doc);

    public interface IEngineCommunication
    {
        event MessageReceivedHandler MessageReceived;

        bool SupportedOnCurrentSystem { get; }
        string InstanceIdentifier { get; }
        void ResetWithCurrentPath();
        void SendMessage(string data);
        void Dispose();
        void NewClient();
        void ClientHasExited();
    }
}
