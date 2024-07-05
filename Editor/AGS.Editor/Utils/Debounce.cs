using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace AGS.Editor.Utils
{
    // Found in SO: https://stackoverflow.com/a/47933557
    class Debouncer
    {
        private List<CancellationTokenSource> StepperCancelTokens = new List<CancellationTokenSource>();
        private readonly int MillisecondsToWait;
        private readonly object _lockThis = new object(); // Use a locking object to prevent the debouncer to trigger again while the func is still running

        public Debouncer(int millisecondsToWait = 150)
        {
            this.MillisecondsToWait = millisecondsToWait;
        }

        public void Debounce(Action func)
        {
            CancelAllStepperTokens();
            var newTokenSrc = new CancellationTokenSource();
            lock (_lockThis)
            {
                StepperCancelTokens.Add(newTokenSrc);
            }
            Task.Delay(MillisecondsToWait, newTokenSrc.Token).ContinueWith(task => // Create new request
            {
                if (!newTokenSrc.IsCancellationRequested) // if it hasn't been cancelled
                {
                    CancelAllStepperTokens(); // Cancel any that remain (there shouldn't be any)
                    StepperCancelTokens = new List<CancellationTokenSource>(); // set to new list
                    lock (_lockThis)
                    {
                        func(); // run
                    }
                }
            }, TaskScheduler.FromCurrentSynchronizationContext());
        }

        private void CancelAllStepperTokens()
        {
            foreach (var token in StepperCancelTokens)
            {
                if (!token.IsCancellationRequested)
                {
                    token.Cancel();
                }
            }
        }
    }
}
