package rvls.jni;

import java.io.File;

public class Frontend  {
    public static native long newContext(String workspace);
    public static native void deleteContext(long handle);

    public static native void spikeDebug(long handle, Boolean enable);
    public static native void spikeLogCommit(long handle, Boolean enable) ;

    public static native void newCpuMemoryView(long handle, int viewId, long readIds, long writeIds);
    public static native void newCpu(long handle, int hartId, String isa, String priv, int physWidth, int memoryViewId);
    public static native void loadElf(long handle, long offset, String path);
    public static native void loadBin(long handle, long offset, String path);
    public static native void setPc(long handle, int hartId, long pc);
    public static native void writeRf(long handle, int hardId, int rfKind, int address, long data);
    public static native void readRf(long handle, int hardId, int rfKind, int address, long data);
    public static native void commit(long handle, int hartId, long pc);
    public static native void trap(long handle, int hartId, boolean interrupt, int code);
    public static native void ioAccess(long handle, int hartId, Boolean write, long address, long data, int mask, int size, boolean error);
    public static native void setInterrupt(long handle, int hartId, int intId, boolean value);
    public static native void addRegion(long handle, int hartId, int kind, long base, long size);
    public static native void loadExecute(long handle, int hartId, long id, long addr, long len, long data);
    public static native void loadCommit(long handle, int hartId, long id);
    public static native void loadFlush(long handle, int hartId);
    public static native void storeCommit(long handle, int hartId, long id, long addr, long len, long data);
    public static native void storeBroadcast(long handle, int hartId, long id);
    public static native void storeConditional(long handle, int hartId, boolean failure);
    public static native void time(long handle, long value);
    public static native void flush(long handle);
    public static native void close(long handle);

    static {
        System.load(new File("ext/rvls/build/apps/rvls").getAbsolutePath());
    }
}
