/* From http://blog.paranoidferret.com/index.php/2007/07/25/interprocess-communication-using-named-pipes-in-csharp/
 * 
 * NOTE: .NET 3.5 adds System.IO.Pipes with native support for this, but 
 * we need to support .NET 2.0 for now
 * */
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Threading;
using System.IO;

namespace AGS.Editor
{
    enum NamedPipeType
    {
        Read = 1,
        Write = 2,
        Both = 3
    }

    class NamedPipesServer
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern SafeFileHandle CreateNamedPipe(
           String pipeName,
           uint dwOpenMode,
           uint dwPipeMode,
           uint nMaxInstances,
           uint nOutBufferSize,
           uint nInBufferSize,
           uint nDefaultTimeOut,
           IntPtr lpSecurityAttributes);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern int ConnectNamedPipe(
           SafeFileHandle hNamedPipe,
           IntPtr lpOverlapped);

        public const uint DUPLEX = (0x00000003);
        public const uint FILE_FLAG_OVERLAPPED = (0x40000000);

        public class Client
        {
            public bool abort = false;
            public SafeFileHandle handle;
            public FileStream stream;
            public NamedPipesServer server;
        }

        public delegate void MessageReceivedHandler(Client client, string message);

        public event MessageReceivedHandler MessageReceived;
        public const int BUFFER_SIZE = 4096;

        private string _pipeName;
        private NamedPipeType _pipeType;
        private Thread listenThread;
        private bool running;
        private List<Client> _clients;

        public string PipeName
        {
            get { return this._pipeName; }
        }

        public NamedPipeType PipeType
        {
            get { return this._pipeType; }
        }

        public bool Running
        {
            get { return this.running; }
            set { this.running = value; }
        }

        public int ClientCount
        {
            get
            {
                int clientCount = 0;
                lock (_clients)
                {
                    foreach (Client client in this._clients)
                    {
                        if (client.stream != null)
                        {
                            clientCount++;
                        }
                    }
                }
                return clientCount;
            }
        }

        public NamedPipesServer(string pipeName, NamedPipeType pipeType)
        {
            this._pipeName = pipeName;
            this._pipeType = pipeType;
            this._clients = new List<Client>();
        }

        /// <summary>
        /// Starts the pipe server
        /// </summary>
        public void Start()
        {
            this.running = true;

            //start the listening thread
            this.listenThread = new Thread(new ParameterizedThreadStart(ListenForClients));
            this.listenThread.IsBackground = true;
            this.listenThread.Start(this);
        }

        public void Stop()
        {
            this.running = false;
            this.listenThread.Abort();
        }

        /// <summary>
        /// Listens for client connections
        /// </summary>
        private static void ListenForClients(object currentServer)
        {
            NamedPipesServer server = (NamedPipesServer)currentServer;
            try
            {
                while (server.Running)
                {
                    SafeFileHandle clientHandle = CreateNamedPipe(
                         server.PipeName,
                         (uint)server.PipeType,
                         0,
                         255,
                         BUFFER_SIZE,
                         BUFFER_SIZE,
                         0,
                         IntPtr.Zero);

                    //could not create named pipe
                    if (clientHandle.IsInvalid)
                        throw new InvalidOperationException("Unable to create pipe");

                    int success = ConnectNamedPipe(clientHandle, IntPtr.Zero);

                    //could not connect client
                    if (success == 0)
                        throw new InvalidOperationException("Unable to connect pipe to client");

                    Client client = new Client();
                    client.handle = clientHandle;
                    client.server = server;

                    lock (server._clients)
                    {
                        server._clients.Add(client);
                    }

                    Thread readThread = new Thread(new ParameterizedThreadStart(ClientPollingThread));
                    readThread.IsBackground = true;
                    readThread.Name = "Named pipe client thread";
                    readThread.Start(client);
                }
            }
            finally
            {
                server.Running = false;
            }
        }

        public void OnMessageReceived(Client client, string message)
        {
            if (this.MessageReceived != null)
                this.MessageReceived(client, message);
        }

        /// <summary>
        /// Reads incoming data from connected clients
        /// </summary>
        /// <param name="clientObj"></param>
        private static void ClientPollingThread(object clientObj)
        {
            Client client = (Client)clientObj;
            FileAccess streamType = FileAccess.ReadWrite;
            if (client.server.PipeType == NamedPipeType.Read) streamType = FileAccess.Read;
            if (client.server.PipeType == NamedPipeType.Write) streamType = FileAccess.Write;

            client.stream = new FileStream(client.handle, streamType, BUFFER_SIZE, false);
            byte[] buffer = new byte[BUFFER_SIZE];

            try
            {
                while ((client.server.Running) && (!client.abort))
                {
                    if (client.server.PipeType == NamedPipeType.Write)
                    {
                        System.Threading.Thread.Sleep(10);
                    }
                    else
                    {
                        int bytesRead = client.stream.Read(buffer, 0, BUFFER_SIZE);

                        //client has disconnected
                        if (bytesRead == 0)
                            break;

                        //fire message received event
                        client.server.OnMessageReceived(client, Encoding.Default.GetString(buffer, 0, bytesRead));
                    }
                }
            }
            finally
            {
                //clean up resources
                client.stream.Close();
                client.handle.Close();
                lock (client.server._clients)
                {
                    if (client.server._clients.Contains(client))
                        client.server._clients.Remove(client);
                }
            }
        }

        /// <summary>
        /// Sends a message to all connected clients
        /// </summary>
        /// <param name="message">the message to send</param>
        public void SendMessage(string message)
        {
            byte[] messageBuffer = Encoding.Default.GetBytes(message);
            lock (this._clients)
            {
                foreach (Client client in this._clients)
                {
                    client.stream.Write(messageBuffer, 0, messageBuffer.Length);
                    client.stream.Flush();
                }
            }
        }

        public void RemoveAllClients()
        {
            lock (_clients)
            {
                foreach (Client client in this._clients)
                {
                    client.abort = true;
                }
                this._clients.Clear();
            }
        }
    }
}
