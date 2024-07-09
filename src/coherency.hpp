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
        Access *previous, *next;

        bool executed = false, commited = false, broadcasted = false;

        inline void bypass(Access &load){
            if(load.addr < this->addr + this->len &&
               this->addr < load.addr + load.len){
                //do bypass
                u64 startAt = max(load.addr, this->addr);
                u64 endAt = min(load.addr + load.len, this->addr + this->len);
                for(u64 addr = startAt; addr < endAt; addr++){
                    load.bytes[addr - load.addr] = this->bytes[addr - this->addr];
                }
            }
        }

        void clear(){
        	executed = false;
        	commited = false;
        	broadcasted = false;
			valid = false;
        }
    };

    u64 loadsInflightCount;
    vector<Access*> loadsInflight;

//    u64 storesInflightCount;
//    vector<Access*> storesInflight;

    Access *storeHead, *storeLast;


    vector<Access> loads;
    vector<Access> stores;
    Access *storeFresh, *loadFresh;
    Memory &memory;

    //CPU interface
    void loadExecute(u64 id, u64 addr, size_t len, const u8* bytes);
    void loadCommit(u64 id);
    void loadFlush();
    void storeExecute(u64 id, u64 addr, size_t len, const u8* bytes);
    void storeCommit(u64 id);
    void storeBroadcast(u64 id);

    //Spike interface
    void fetch(u64 address,u32 length, u8 *data);
    void mmu(u64 address,u32 length, u8 *data);
    void load(u64 address,u32 length, u8 *data);
    void store(u64 address,u32 length, const u8 *data);
    void step();
};
