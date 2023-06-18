#if !DEBUG
// TODO: Figure out why this does not work
//#define USE_MEMORY_MODULE
#endif

using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;

namespace StarcraftMapContentSeeker0.lib {
    class TheLib {

#if !USE_MEMORY_MODULE

        [DllImport("kernel32.dll")]
        public static extern IntPtr LoadLibrary(string dllToLoad);

        [DllImport("kernel32.dll")]
        public static extern IntPtr GetProcAddress(IntPtr hModule, IntPtr procOrdinal);

        [DllImport("kernel32.dll")]
        public static extern bool FreeLibrary(IntPtr hModule);

#endif
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ProcessFunc(IntPtr input, IntPtr output, IntPtr outputSize, IntPtr writtenLength);

        ProcessFunc _Process = null;

        public TheLib() {
#if USE_MEMORY_MODULE
            byte[] data = StarcraftMapContentSeeker0.Properties.Resources.STRExtractor;
            MemoryModule mem = new MemoryModule(data);
            _Process = (ProcessFunc)mem.GetDelegateFromFuncName(0, typeof(ProcessFunc));
#else
            IntPtr pDll = LoadLibrary("STRExtractor.dll");
            if (pDll == IntPtr.Zero) {
                MessageBox.Show("Missing STRExtractor.dll");
            } else { 
                IntPtr pAddressOfFunctionToCall0 = GetProcAddress(pDll, (IntPtr)1);
                if (pAddressOfFunctionToCall0 == IntPtr.Zero) {
                    MessageBox.Show("Missing export in STRExtractor.dll");
                } else {
                    _Process = (ProcessFunc)Marshal.GetDelegateForFunctionPointer(pAddressOfFunctionToCall0, typeof(ProcessFunc));
                }
            }
#endif
        }

        public int ReadSTR(string inputFile, ref byte[] target) {
            unsafe {
                byte[] inputFileBytes = Encoding.UTF8.GetBytes(inputFile);
                fixed(byte* inputFileBytesPtr = inputFileBytes) {
                    fixed (byte* targetPtr = target) {
                        UInt32 written = 0;
                        IntPtr sizePtr = new IntPtr(&written);
                        _Process(new IntPtr(inputFileBytesPtr), new IntPtr(targetPtr), new IntPtr(target.Length), sizePtr);
                        return (int)written;
                    }
                }
            }
        }
    }
}
