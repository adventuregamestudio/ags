using System;
using System.Collections.Concurrent;
using System.Collections.Generic;

namespace AGS.Editor
{
    /// <summary>
    /// A naive implementation of the circular buffer: a queue of a
    /// fixed size that overwrites first item if filled to full capacity.
    /// Extends ConcurrentQueue.
    /// </summary>
    public class ConcurrentCircularBuffer<T> : ConcurrentQueue<T>
    {
        public readonly int Capacity;

        public ConcurrentCircularBuffer(IEnumerable<T> collection, int capacity)
            : base(collection)
        {
            Capacity = capacity;
        }

        public new void Enqueue(T item)
        {
            while (Count + 1 > Capacity)
            {
                T discard;
                base.TryDequeue(out discard); // should always succeed unless there's no items
            }

            base.Enqueue(item);
        }

        /// <summary>
        /// Helper method for dequeueing and discarding an item.
        /// </summary>
        public bool Discard()
        {
            T discard;
            return base.TryDequeue(out discard);
        }

        /// <summary>
        /// Helper method for clearing the buffer.
        /// </summary>
        public void TryClear()
        {
            while (Count > 0)
            {
                T discard;
                base.TryDequeue(out discard);
            }
        }
    }
}
