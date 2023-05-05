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
        private string _cachedText;

        public string Text { get; private set; }
        public LogGroup Group { get; private set; }
        public LogLevel Level { get; private set; }
        public int TextLength
        {
            get
            {
                if (_cachedText == null)
                    CacheText();
                return _cachedText.Length;
            }
        }

        public LogEntry(string text, LogGroup group, LogLevel level)
        {
            Text = text;
            Group = group;
            Level = level;
        }

        public override string ToString()
        {
            if (_cachedText == null)
                CacheText();
            return _cachedText;
        }

        private void CacheText()
        {
            _cachedText = "[" + Group.ToString() + "][" + Level.ToString() + "]: " + Text + "\n";
        }
    }
        
    class LogBuffer
    {
        private const int _BUFFER_SIZE = 4096;
        private bool[,] _filter = new bool[(int)LogGroup.NumGroups + 1, (int)LogLevel.NumLevels + 1];
        private bool _modified;
        private bool _notifyReset; // notify significant changes to the buffer
        private int _popCount; // number of old chars popped
        private int _pushCount; // number of new chars pushed
        System.Timers.Timer _debounceTimer;

        // Full buffer
        private ConcurrentCircularBuffer<LogEntry> _buffer =
            new ConcurrentCircularBuffer<LogEntry>(new List<LogEntry>(_BUFFER_SIZE), _BUFFER_SIZE);
        // Recent entries buffer, cleaned up by the user query;
        // this buffer contains already filtered entries
        private ConcurrentCircularBuffer<LogEntry> _recentBuffer =
            new ConcurrentCircularBuffer<LogEntry>(new List<LogEntry>(_BUFFER_SIZE), _BUFFER_SIZE);

        private void PushBack(LogEntry item)
        {
            // Predict buffer will discard an item
            // TODO: bit ugly, may be done more elegantly?
            if (_buffer.Count == _buffer.Capacity)
            {
                LogEntry peekItem;
                if (_buffer.TryPeek(out peekItem) &&
                    GetFilterArr(item.Group, item.Level))
                {
                    _popCount += peekItem.TextLength;
                }
            }

            _buffer.Enqueue(item);

            // Push into the recent entries buffer, if entry passes the filter
            if (GetFilterArr(item.Group, item.Level))
            {
                _recentBuffer.Enqueue(item);
                _pushCount += item.TextLength;
            }
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

        public delegate void LogBufferChangedHandler(object sender, LogBufferEventArgs evArgs);
        public event LogBufferChangedHandler BufferChanged;

        private void RaiseEventBufferChanged()
        {
            BufferChanged?.Invoke(this, new LogBufferEventArgs(_notifyReset, _popCount, _pushCount));
            _modified = false;
            _notifyReset = false;
            _popCount = 0;
            _pushCount = 0;
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
            _notifyReset = true;
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
                RaiseEventBufferChanged();
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

        /// <summary>
        /// Discards whole buffer.
        /// </summary>
        public void Clear()
        {
            if (IsEmpty()) return;

            _buffer.TryClear();
            _recentBuffer.TryClear();

            RaiseEventBufferChanged();
        }

        /// <summary>
        /// Adds a new log entry. If the buffer is full, the oldest entry will be discarded.
        /// </summary>
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

        /// <summary>
        /// Retrieves full buffer contents. Discards "recent" buffer.
        /// </summary>
        public string GetFullText()
        {
            _recentBuffer.TryClear();
            return ToString();
        }

        /// <summary>
        /// Retrieves the "recent" contents, accumulated since the previous query.
        /// Drains "recent" buffer.
        /// </summary>
        public string QueryLastEntries()
        {
            List<LogEntry> bufcopy = new List<LogEntry>(_recentBuffer.Count);
            for (int i = 0; i < _recentBuffer.Count; ++i)
            {
                LogEntry discard;
                if (!_recentBuffer.TryDequeue(out discard))
                    break;
                bufcopy.Add(discard);
            }

            // NOTE: we don't filter here as freshBuffer contains filtered items
            return string.Join("", bufcopy
                .Select(le => le.ToString()));
        }
    }
}
