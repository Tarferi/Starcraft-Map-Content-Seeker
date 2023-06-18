using StarcraftMapContentSeeker0.lib;
using StarcraftMapContentSeeker0.utils;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;

namespace StarcraftMapContentSeeker0 {
   
    public partial class MainWindow : Window {

        private bool regex = false;
        private string search = "";
        private bool recursive = true;

        TheLib lib = null;
        byte[] buffer = new byte[0xffff];
        int bufferLength = 0;

        public MainWindow() {
            InitializeComponent();
            AsyncManager.Bootstrap(this);
            lib = new TheLib();
            fileInput.DirectoryPicker = true;
        }

        void SetEnabled(bool enabled) {
            fileInput.IsEnabled = enabled;
            checkRecursive.IsEnabled = enabled;
            txtSearch.IsEnabled = enabled;
            checkRegex.IsEnabled = enabled;
            btnRun.IsEnabled = enabled;
        }

        private void SetLabel(string text) {
            AsyncManager.OnUIThread(() => {
                lblStatus.Text = text;
            }, ExecutionOption.Blocking);
        }

        private void IncCount() {
            count++;
            AsyncManager.OnUIThread(() => {
                lblScanned.Text = count.ToString();
            }, ExecutionOption.Blocking);
        }

        int count = 0;
        Dictionary<string, int> cache = new Dictionary<string, int>();

        class Entry {
            public string Content { get; set; }
            public string Files { get; set; }
            public int fileCnt;
            public string FilesCnt { get => fileCnt.ToString(); }
            public List<string> files = new List<string>();

            public Entry(string content, string file) {
                Content = content;
                Files = file;
                fileCnt = 1;
            }
        }

        private void AddRow(string foundText, string inWhichFile) {
            int idx = 0;
            if (cache.TryGetValue(foundText, out idx)) {
                AsyncManager.OnUIThread(() => {
                    ((Entry)lstData.Items[idx]).fileCnt++;
                    ((Entry)lstData.Items[idx]).files.Add(inWhichFile);
                    lstData.Items.Refresh();
                }, ExecutionOption.Blocking);
            } else {
                Entry item = new Entry(foundText, inWhichFile);
                item.files.Add(inWhichFile);
                AsyncManager.OnUIThread(() => {
                    cache[foundText] = lstData.Items.Count;
                    lstData.Items.Add(item);
                }, ExecutionOption.Blocking);
            }
        }

        ushort ShortAt(int idx, ref bool error) {
            if (!error) {
                if (idx + 1 < bufferLength) {
                    int x1 = buffer[idx];
                    int x2 = buffer[idx + 1];
                    return (ushort)((x2 << 8) + x1);
                } else {
                    error = true;
                }
            }
            return 0;
        }

        string StringAt(int idx, ref bool error) {
            if (!error) {
                if (idx < bufferLength) {
                    for(int i = idx; i < bufferLength; i++) {
                        if (buffer[i] == 0) {
                            if (i > idx) {
                                return Encoding.UTF8.GetString(buffer, idx, i - idx);
                            } else {
                                return "";
                            }
                        }
                    }
                    error = true;
                }
            }
            return null;
        }

        List<string> localStrings = new List<string>();

        private void ProcessString(string path, string str) {
            if (regex) {
                MatchCollection m = Regex.Matches(str, "(" + search + ")", RegexOptions.IgnoreCase);
                for(int i = 0; i < m.Count; i++) {
                    if (m[i].Success) {
                        string match = m[i].Value;
                        if (!localStrings.Contains(match)) {
                            localStrings.Add(match);
                            AddRow(match, path);
                        }
                    }
                }
            } else {
                if (str.ToLower().Contains(search.ToLower())) {
                    if (!localStrings.Contains(path)) {
                        localStrings.Add(path);
                        AddRow(str, path);
                    }
                }
            }
        }

        private void ProcessFile(string path) {
            SetLabel("Processing " + path);
            bufferLength = lib.ReadSTR(path, ref buffer);
            if (bufferLength > 2) {
                bool error = false;
                ushort strings = ShortAt(0, ref error);
                for(int i = 0; i < strings && !error; i++) {
                    ushort offset = ShortAt((2 * (i + 1)), ref error);
                    if (!error) {
                        string tmp = StringAt(offset, ref error);
                        if (tmp != null) {
                            localStrings.Clear();
                            ProcessString(path, tmp);
                        }
                    }
                }
            }
            IncCount();
        }

        private void ScanDir(string dir) {
            foreach(string file in Directory.GetFiles(dir)) {
                ProcessFile(file);
            }
            if (recursive) {
                foreach (string path in Directory.GetDirectories(dir)) {
                    ScanDir(path);
                }
            }
        }

        private void btnRun_Click(object sender, RoutedEventArgs e) {
            string path = fileInput.Content.Trim();
            cache.Clear();
            count = 0;
            lstData.Items.Clear();
            recursive = checkRecursive.IsChecked.Value;
            search = txtSearch.Text.Trim();
            regex = checkRegex.IsChecked.Value;

            if (path.Length == 0 || !Directory.Exists(path) || search.Length == 0) {
                return;
            }
            SetEnabled(false);
            SetLabel("Ready");

            new AsyncJob(() => {
                ScanDir(path);
                return null;
            }, (res) => {
                SetEnabled(true);
                SetLabel("Ready");
            }).Run();
        }

        private void lstData_SizeChanged(object sender, SizeChangedEventArgs e) {
            ListView listView = sender as ListView;
            GridView gView = listView.View as GridView;

            var workingWidth = listView.ActualWidth - SystemParameters.VerticalScrollBarWidth - gView.Columns[2].Width;
            var col1 = 0.50;
            var col2 = 0.50;

            gView.Columns[0].Width = workingWidth * col1;
            gView.Columns[1].Width = workingWidth * col2;
        }

        private void OpenLink(string link) {
            System.Diagnostics.Process.Start(link);
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e) {
            OpenLink("https://www.wannatalkaboutit.com/");
        }

        private void MenuItem_Click_1(object sender, RoutedEventArgs e) {
            OpenLink("https://www.youtube.com/watch?v=5TTPPXtsd4A&ab_channel=Rima");
        }

        private void MenuItem_Click_2(object sender, RoutedEventArgs e) {
            if(lstData.SelectedItem != null) {
                Entry entry = (Entry)lstData.SelectedItem;
                System.Windows.Forms.Clipboard.SetText(entry.Content);
            }
        }

        private void MenuItem_Click_3(object sender, RoutedEventArgs e) {
            if(lstData.SelectedItem != null) {
                Entry entry = (Entry)lstData.SelectedItem;
                StringBuilder sb = new StringBuilder();
                bool first = true;
                foreach(string f in entry.files) {
                    if (first) {
                        first = false;
                    } else {
                        sb.Append("\r\n");
                    }
                    sb.Append(f);
                }
                System.Windows.Forms.Clipboard.SetText(sb.ToString());
            }
        
        }

        private void MenuItem_Click_4(object sender, RoutedEventArgs e) {
            Application.Current.Shutdown();
            Environment.Exit(0);
        }


        private void MenuItem_Click_5(object sender, RoutedEventArgs e) {
            ui.wndAbout abt = new ui.wndAbout();
            abt.ShowDialog();
        }
    }
}
