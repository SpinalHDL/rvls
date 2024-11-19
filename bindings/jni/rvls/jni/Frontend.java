package rvls.jni;

import java.io.File;

public class Frontend  {
    public static native long newDisassemble(int xlen);
    public static native void deleteDisassemble(long handle);
    public static native String disassemble(long handle, long instruction);
    
    public static native long newContext(String workspace);
    public static native void deleteContext(long handle);

    public static native void spikeDebug(long handle, boolean enable);
    public static native void spikeLogCommit(long handle, boolean enable) ;

    public static native void newCpuMemoryView(long handle, int viewId, long readIds, long writeIds);
    public static native void newCpu(long handle, int hartId, String isa, String priv, int physWidth, int pmpNum, int memoryViewId);
    public static native void loadElf(long handle, long offset, String path);
    public static native void loadBin(long handle, long offset, String path);
    public static native void loadBytes(long handle, long offset, byte[] bytes);
    public static native void setPc(long handle, int hartId, long pc);
    public static native void writeRf(long handle, int hardId, int rfKind, int address, long data);
    public static native void readRf(long handle, int hardId, int rfKind, int address, long data);
    public static native boolean commit(long handle, int hartId, long pc);
    public static native boolean trap(long handle, int hartId, boolean interrupt, int code);
    public static native String getLastErrorMessage(long handle);
    public static native void ioAccess(long handle, int hartId, boolean write, long address, long data, int mask, int size, boolean error);
    public static native void setInterrupt(long handle, int hartId, int intId, boolean value);
    public static native void addRegion(long handle, int hartId, int kind, long base, long size);
    public static native boolean loadExecute(long handle, int hartId, long id, long addr, long len, long data);
    public static native boolean loadCommit(long handle, int hartId, long id);
    public static native boolean loadFlush(long handle, int hartId);
    public static native boolean storeExecute(long handle, int hartId, long id, long addr, long len, long data);
    public static native boolean storeCommit(long handle, int hartId, long id);
    public static native boolean storeBroadcast(long handle, int hartId, long id);
    public static native boolean storeConditional(long handle, int hartId, boolean failure);
    public static native void time(long handle, long value);
    public static native void flush(long handle);
    public static native void close(long handle);

    static {
        System.load(new File("ext/rvls/build/apps/rvls.so").getAbsolutePath());
    }
}
