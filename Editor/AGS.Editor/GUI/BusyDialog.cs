using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace AGS.Editor
{
    public interface IWorkProgress
    {
        int? Total { get; set; }
        int? Current { get; set; }
        string Message { get; set; }
    }

    public partial class BusyDialog : Form, IWorkProgress
    {
        public delegate object ProcessingHandler(IWorkProgress progress, object parameter);

        private System.Windows.Forms.Timer _timer;
        private int _tickCount = 0;
        private ProcessingHandler _handler;
        private object _parameter;
        private volatile bool _threadFinished;
        private Exception _exceptionThrownOnThread;
        private object _result;
        private bool _allowClose = false;
        private int? _progressTotal = null;
        private int? _progressCurrent = null;
        private string _originalMessage;

        private static Bitmap[] _icons = null;

        public BusyDialog(string message, ProcessingHandler handler, object parameter)
        {
            InitializeComponent();
            _originalMessage = message;
            lblMessage.Text = message;
            _handler = handler;
            _parameter = parameter;

            if (_icons == null)
            {
                _icons = new Bitmap[8];
                for (int i = 0; i < _icons.Length; i++)
                {
                    _icons[i] = Resources.ResourceManager.GetIcon("busy_blue-circle_" + (i + 1) + ".ico").ToBitmap();
                }
            }
        }

        public Exception ExceptionThrown
        {
            get { return _exceptionThrownOnThread; }
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static object Show(string message, ProcessingHandler handler, object parameter)
        {
            object resultToReturn = null;
            BusyDialog dialog = new BusyDialog(message, handler, parameter);
            try
            {
                dialog.ShowDialog();
                resultToReturn = dialog.Result;
                if (dialog.ExceptionThrown != null)
                {
                    // Re-throw the same type of exception
                    ConstructorInfo constructor = dialog.ExceptionThrown.GetType().GetConstructor(new Type[] { dialog.ExceptionThrown.GetType() });
                    if (constructor != null)
                    {
                        // Copy constructor
                        throw (Exception)constructor.Invoke(new object[] { dialog.ExceptionThrown });
                    }
                    else
                    {
                        constructor = dialog.ExceptionThrown.GetType().GetConstructor(new Type[] { typeof(string), typeof(Exception) });
                        if (constructor == null)
                        {
                            // default constructor
                            throw new AGSEditorException(dialog.ExceptionThrown.Message, dialog.ExceptionThrown);
                        }
                        else
                        {
                            throw (Exception)constructor.Invoke(new object[] { dialog.ExceptionThrown.Message, dialog.ExceptionThrown });
                        }
                    }
                }
            }
            finally
            {
                dialog.Dispose();
            }
            return resultToReturn;
        }

        public object Result
        {
            get { return _result; }
        }

        private void UpdateMessageLabel()
        {
            string progrText = "";
            if (_progressTotal.HasValue && _progressCurrent.HasValue)
                progrText = string.Format("({0} / {1})", _progressCurrent, _progressTotal);
            else if (_progressTotal.HasValue)
                progrText = string.Format("(- / {0})", _progressTotal);
            else if (_progressCurrent.HasValue)
                progrText = string.Format("({0} / -)", _progressCurrent);
            lblMessage.Text = _originalMessage + " " + progrText;
        }

        public string Message
        {
            get { return lblMessage.Text; }
            set
            {
                _originalMessage = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public int? Total
        {
            get { return _progressTotal; }
            set
            {
                _progressTotal = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public int? Current
        {
            get { return _progressCurrent; }
            set
            {
                _progressCurrent = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        private void BusyDialog_Load(object sender, EventArgs e)
        {
            _timer = new System.Windows.Forms.Timer();
            _timer.Interval = 100;
            _timer.Tick += new EventHandler(_timer_Tick);
            _timer.Start();
            _threadFinished = false;
            _exceptionThrownOnThread = null;
            _result = null;
            btnImage.Image = _icons[0];
            new Thread(new ThreadStart(RunHandlerOnThread)).Start();
        }

        private void RunHandlerOnThread()
        {
            try
            {
                _result = _handler(this, _parameter);
            }
            catch (Exception ex)
            {
                _exceptionThrownOnThread = ex;
            }
            _threadFinished = true;
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            if (_threadFinished)
            {
                _timer.Enabled = false;
                _allowClose = true;
                this.Close();
                return;
            }

            _tickCount++;
            btnImage.Image = _icons[_tickCount % _icons.Length];
        }

        private void BusyDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (!_allowClose)
            {
                e.Cancel = true;
            }
        }
    }
}