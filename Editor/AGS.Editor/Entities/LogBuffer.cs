using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Timers;
using AGS.Types.Enums;

namespace AGS.Editor
{
    class LogEntry
    {
        public string Text { get; private set; }
        public LogGroup Group { get; private set; }
        public LogLevel Level { get; private set; }

        public LogEntry(string text, LogGroup group, LogLevel level)
        {
            Text = text;
            Group = group;
            Level = level;
        }

        public override string ToString()
        {
            return "[" + Group.ToString() + "][" + Level.ToString() + "]: " + Text + "\n";
        }
    }
        
    class LogBuffer
    {
        private const int _BUFFER_SIZE = 4096;
        private bool[,] _filter = new bool[(int)LogGroup.NumGroups + 1, (int)LogLevel.NumLevels + 1];
        private bool _modified;
        System.Timers.Timer _debounceTimer;

        private ConcurrentCircularBuffer<LogEntry> _buffer =
            new ConcurrentCircularBuffer<LogEntry>(new List<LogEntry>(_BUFFER_SIZE), _BUFFER_SIZE);

        private void PopBack()
        {
            _buffer.Discard();
        }

        private void PushBack(LogEntry item)
        {
            _buffer.Enqueue(item);
        }

        private bool IsEmpty()
        {
            return !_buffer.Any();
        }

        private void SetFilterArr(int int_group, int int_level, bool value)
        {
            _filter[int_group + 1, int_level + 1] = value;
        }

        private void SetFilterArr(LogGroup group, int int_level, bool value)
        {
            SetFilterArr((int)group, int_level, value);
        }

        private bool GetFilterArr(int int_group, int int_level)
        {
            return _filter[int_group + 1, int_level + 1];
        }

        private bool GetFilterArr(LogGroup group, int int_level)
        {
            return GetFilterArr((int)group, int_level);
        }

        private bool GetFilterArr(LogGroup group, LogLevel level)
        {
            return GetFilterArr((int)group, (int)level);
        }

        public event EventHandler ValueChanged;
        private void RaiseEventValueChanged(EventArgs e)
        {
            ValueChanged?.Invoke(this, e);
        }

        protected virtual void OnValueChanged(EventArgs e)
        {
            RaiseEventValueChanged(e);
        }

        public void SetLogLevel(LogGroup group, LogLevel level)
        {
            int i = 0;
            for(; i<(int)level + 1; i++)
            {
                SetFilterArr(group, i, true);
            }
            for (; i < (int)LogLevel.NumLevels; i++)
            {
                SetFilterArr(group, i, false);
            }
            DebounceChange();
        }

        public LogLevel GetLogLevel(LogGroup group)
        {
            for (int i = (int)LogLevel.NumLevels-1; i > 0; i--)
            {
                if (GetFilterArr(group, i)) return (LogLevel)i;
            }
            return LogLevel.Debug;
        }

        public LogBuffer()
        {
            SetLogLevel(LogGroup.Main, LogLevel.Debug);
            SetLogLevel(LogGroup.Game, LogLevel.Debug);
            SetLogLevel(LogGroup.Script, LogLevel.Debug);
            SetLogLevel(LogGroup.SprCache, LogLevel.None);
            SetLogLevel(LogGroup.ManObj, LogLevel.None);
            SetLogLevel(LogGroup.SDL, LogLevel.None);
        }

        private void elapse(object sender, ElapsedEventArgs e)
        {
            if(_modified)
            {
                _modified = false;
                RaiseEventValueChanged(new EventArgs());
            }
        }

        private void DebounceChange()
        {
            if(_debounceTimer != null)
            {
                _debounceTimer.Enabled = false;
                _debounceTimer.Stop();
                _debounceTimer.Dispose();
                _debounceTimer = null;
            }
            _debounceTimer = new System.Timers.Timer(10);
            _debounceTimer.Elapsed += new System.Timers.ElapsedEventHandler(elapse); // subscribing to the elapse event
            _debounceTimer.Start();
        }

        public void Clear()
        {
            if (IsEmpty()) return;

            _buffer.TryClear();

            RaiseEventValueChanged(new EventArgs());
        }

        public void Append(string text, LogGroup group, LogLevel level)
        {
            PushBack(new LogEntry(text, group, level));
            _modified = true;
            DebounceChange();
        }

        public override string ToString()
        {
            List<LogEntry> bufcopy = _buffer.ToList();

            return string.Join("", bufcopy
                .Where(le => GetFilterArr(le.Group, le.Level))
                .Select(le => le.ToString()));
        }
    }
}
