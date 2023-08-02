#pragma once
#include "stddef.h"
#include "type.h"
#include "memory.hpp"
#include <vector>

using namespace std;

class MemoryView{
public:

};

class DirectMemoryView : public MemoryView{
public:

};

class CpuMemoryView : public MemoryView{
public:
    CpuMemoryView(Memory &memory, u64 readIds, u64 writeIds);

    class Access{
    public:
        u64 addr;
        size_t len;
        u8 bytes[8];
        u64 userId;
        bool valid = false;
    };

    u64 loadsInflightCount;
    vector<Access*> loadsInflight;
    vector<Access> loads;
    vector<Access> stores;
    Access *storeFresh, *loadFresh;
    Memory &memory;

    //CPU interface
    void loadExecute(u64 id, u64 addr, size_t len, const u8* bytes);
    void loadCommit(u64 id);
    void loadFlush();
    void storeCommit(u64 id, u64 addr, size_t len, const u8* bytes);
    void storeBroadcast(u64 id);

    //Spike interface
    void fetch(u32 address,u32 length, u8 *data);
    void load(u32 address,u32 length, u8 *data);
    void store(u32 address,u32 length, const u8 *data);
    void step();
};
