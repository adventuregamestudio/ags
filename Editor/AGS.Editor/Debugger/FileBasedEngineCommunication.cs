using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;

namespace AGS.Editor
{
    /// <summary>
    /// A niave first implementation just using a file. This should
    /// be improved to use named pipes or something better.
    /// </summary>
    public class FileBasedEngineCommunication : IEngineCommunication
    {
        public event MessageReceivedHandler MessageReceived;

        private const string DIRECTORY_FOR_FILES = ".";
        private static readonly string SEND_MESSAGE_FILE = Path.Combine(DIRECTORY_FOR_FILES, "dbgsend.tmp");
        private const string RECIEVE_MESSAGE_FILE = "dbgrecv.tmp";

        private FileSystemWatcher _watcher;

        public FileBasedEngineCommunication() 
        {
        }

        public string InstanceIdentifier
        {
            get { return "None"; }
        }

		public bool SupportedOnCurrentSystem
		{
			get
			{
				// The FileSystemWatcher doesn't work on Win9x
				return (System.Environment.OSVersion.Platform == PlatformID.Win32NT);
			}
		}

        public void ResetWithCurrentPath()
        {
            if (_watcher != null)
            {
                _watcher.Created -= new FileSystemEventHandler(_watcher_Changed);
                _watcher.Dispose();
            }

            if (File.Exists(SEND_MESSAGE_FILE))
            {
                File.Delete(SEND_MESSAGE_FILE);
            }
            if (File.Exists(Path.Combine(DIRECTORY_FOR_FILES, RECIEVE_MESSAGE_FILE)))
            {
                File.Delete(Path.Combine(DIRECTORY_FOR_FILES, RECIEVE_MESSAGE_FILE));
            }

            string path = Path.Combine(Directory.GetCurrentDirectory(), DIRECTORY_FOR_FILES);
            _watcher = new FileSystemWatcher(path, RECIEVE_MESSAGE_FILE);
            _watcher.Changed += new FileSystemEventHandler(_watcher_Changed);
            _watcher.NotifyFilter = NotifyFilters.LastWrite;
            _watcher.EnableRaisingEvents = true;
        }

        private void _watcher_Changed(object sender, FileSystemEventArgs e)
        {
			string data = null;
            bool succeeded = false;
            XmlDocument doc = null;
            bool xmlFailedAlready = false;
            while (!succeeded)
            {
                try
                {
                    StreamReader sr = new StreamReader(e.FullPath);
                    data = sr.ReadToEnd();
                    sr.Close();
                    doc = new XmlDocument();
                    doc.LoadXml(data);
                    succeeded = true;
                }
                catch (FileNotFoundException)
                {
                    return;
                }
                catch (IOException)
                {
                    System.Threading.Thread.Sleep(10);
                }
                catch (UnauthorizedAccessException)
                {
                    System.Threading.Thread.Sleep(10);
                }
                catch (XmlException ex)
                {
                    if (xmlFailedAlready)
                    {
                        throw new XmlException("Unable to load message: " + data, ex);
                    }
                    System.Threading.Thread.Sleep(100);
                    xmlFailedAlready = true;
                }
            }

            try
            {
                while (File.Exists(e.FullPath))
                {
                    try
                    {
                        File.Delete(e.FullPath);
                    }
                    catch (IOException)
                    {
                        // Unable to delete file
                        System.Threading.Thread.Sleep(50);
                    }
                    catch (UnauthorizedAccessException)
                    {
                        // File in use (access denied)
                        System.Threading.Thread.Sleep(50);
                    }
                }
            }
            catch { }

            if (MessageReceived != null)
            {
                MessageReceived(doc);
            }
        }

        public void SendMessage(string data)
        {
            int timeout = 0;
            while ((File.Exists(SEND_MESSAGE_FILE)) && (timeout < 1000))
            {
                System.Threading.Thread.Sleep(3);
                timeout++;
            }

            StreamWriter sw = new StreamWriter(SEND_MESSAGE_FILE, false, Encoding.Default);
            sw.Write(data);
            sw.Close();
        }

        public void ClientHasExited() { }

        public void NewClient() { }

        public void Dispose()
        {
        }
    }
}
