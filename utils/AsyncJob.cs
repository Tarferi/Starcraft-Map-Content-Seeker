﻿using System;
using System.Collections.Concurrent;
using System.Threading;
using System.Windows;
using System.Windows.Threading;
using System.Threading.Tasks;

namespace StarcraftMapContentSeeker0.utils {

    class AsyncJob {

        private Func<Object> runAsync;
        private Action<Object> finishSync;

        public AsyncJob(Func<Object> runAsync, Action<Object> finishSync) {
            this.runAsync = runAsync;
            this.finishSync = finishSync;
        }

        public void Run() {
            AsyncManager.GetInstance().AddJob(ExecuteAsync);
        }

        private void ExecuteAsync(Action<Action> runSync) {
            Object result = null;
            try {
                result = runAsync();
            } catch (Exception e) {
                result = e;
            }
            try {
                Action runS = () => {
                    try {
                        this.finishSync(result);
                    } catch (Exception) {
                    }
                };
                runSync(runS);
            } catch (Exception) {
            }
        }

    }

    public enum ExecutionOption {
        Blocking,
        DoOtherJobsWhileBlocking,
        NonBlocking
    }

    public class AsyncManager {

        private static AsyncManager instance = new AsyncManager();

        BlockingCollection<Action<Action<Action>>> jobs = new BlockingCollection<Action<Action<Action>>>();

        private MainWindow mc;

        public static void Bootstrap(MainWindow mc) {
            GetInstance().mc = mc;
        }

        public void Stop() {
            jobs.Add(null);
        }

        private void Worker() {
            Action<Action> runSync = (acx) => {
                mc.Dispatcher.Invoke(acx);
            };

            while (true) {
                Action<Action<Action>> job = jobs.Take();
                if (job == null) {
                    return;
                } else {
                    try {
                        job(runSync);
                    } catch (Exception) {
                    }
                }
            }
        }

        private AsyncManager() {
            Thread thread = new Thread(new ThreadStart(Worker));
            thread.Start();
        }

        public static void OnUIThread(Action a, ExecutionOption opts) {
            if (Application.Current == null) {
                //ErrorMessage.Show("Application.Current is not available");
            } else if (Application.Current.Dispatcher == null) {
                //ErrorMessage.Show("Application.Current.Dispatcher is not available");
            } else if (Dispatcher.CurrentDispatcher == null) {
                //ErrorMessage.Show("Dispatcher.CurrentDispatcher is not available");
            } else {
                if (Application.Current.Dispatcher == Dispatcher.CurrentDispatcher) {
                    a();
                } else {
                    switch (opts) {
                        case ExecutionOption.Blocking:
                            Application.Current.Dispatcher.Invoke(a);
                            break;

                        case ExecutionOption.DoOtherJobsWhileBlocking:
                            Application.Current.Dispatcher.InvokeAsync(() => {
                                a();
                                GetInstance().jobs.Add(null);
                            });
                            GetInstance().Worker();
                            break;
                        case ExecutionOption.NonBlocking:
                            Application.Current.Dispatcher.InvokeAsync(a);
                            break;
                    }
                }
            }
        }

        public static AsyncManager GetInstance() {
            return instance;
        }

        public void AddJob(Action<Action<Action>> act) {
            jobs.Add(act);
        }
    }
}



