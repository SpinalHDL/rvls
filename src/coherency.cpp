#include "coherency.hpp"
#include <string.h>
#include "global.hpp"

CpuMemoryView::CpuMemoryView(Memory &memory, u64 readIds, u64 writeIds) : memory(memory){
    loadsInflightCount = 0;
    loadsInflight.resize(readIds);
    loads.resize(readIds);
    stores.resize(writeIds);
    storeFresh = NULL;
    loadFresh = NULL;
}


//Nax interface
void CpuMemoryView::loadExecute(u64 id, u64 addr, size_t len, const u8* bytes){
    auto &load = loads[id];
    if(len > 8) throw std::runtime_error("Load len to big ???");
    load.addr = addr;
    load.len = len;
    //memcpy(load.bytes, bytes, len); //TODO check data loaded for dut
    memory.read(addr, len, load.bytes);
    if(!load.valid) {
        load.userId = loadsInflightCount;
        loadsInflight[loadsInflightCount] = &load;
        loadsInflightCount += 1;
        load.valid = true;
    }
}

void CpuMemoryView::loadCommit(u64 id){
    auto &load = loads[id];
    if(!load.valid) throw std::runtime_error("Load was valid ???");
    loadsInflightCount -= 1;
    loadsInflight[load.userId] = loadsInflight[loadsInflightCount];
    loadsInflight[load.userId]->userId = load.userId;
    if(loadFresh) throw std::runtime_error("loadFresh was valid ???");
    loadFresh = &load;
}

void CpuMemoryView::loadFlush(){
    for(u64 i = 0; i < loadsInflightCount;i++){
        loadsInflight[i]->valid = false;
    }
    loadsInflightCount = 0;
}

void CpuMemoryView::storeCommit(u64 id, u64 addr, size_t len, const u8* bytes){
    auto &store = stores[id];
//    if(store.valid) throw std::runtime_error("Store was valid ???");
    if(len > 8) throw std::runtime_error("Store len to big ???");
    store.valid = true;
    store.addr = addr;
    store.len = len;
    memcpy(store.bytes, bytes, len);
    if(storeFresh) throw std::runtime_error("storeFresh was valid ???");
    storeFresh = &store;

    //TODO we should probably used ref data instead of dut data to bypass stuff XD
    for(u64 i = 0; i < loadsInflightCount;i++){
        auto &load = *loadsInflight[i];
        if(!load.valid)
            throw std::runtime_error("load wasn't valid wuuut ???");
        if(load.addr < store.addr + store.len &&
           store.addr < load.addr + load.len){
            //do bypass
            u64 startAt = max(load.addr, store.addr);
            u64 endAt = min(load.addr + load.len, store.addr + store.len);
            for(u64 addr = startAt; addr < endAt; addr++){
                load.bytes[addr - load.addr] = store.bytes[addr - store.addr];
            }
        }
    }
}

void CpuMemoryView::storeBroadcast(u64 id){
    auto &store = stores[id];
    if(!store.valid) throw std::runtime_error("Store wasn't valid ???");
    memory.write(store.addr, store.len, store.bytes);
    store.valid = false;
}


//Spike interface
void CpuMemoryView::load(u32 address,u32 length, u8 *data){
    if(!loadFresh) throw std::runtime_error("loadFresh wasn't was NULL on check ???");
    if(!loadFresh->valid) throw std::runtime_error("load wasn't valid on check ???");
    auto &load = *loadFresh; loadFresh = NULL;
    assertEq("Bad load addr", load.addr, address);
    assertEq("Bad load length", load.len, length);
    memcpy(data, load.bytes, load.len);
    load.valid = false;
}

void CpuMemoryView::store(u32 address,u32 length, const u8 *data){
    if(!storeFresh) throw std::runtime_error("storeFresh wasn't valid on check ???");
    if(!storeFresh->valid) throw std::runtime_error("Store wasn't valid on check ???");
    auto &store = *storeFresh; storeFresh = NULL;
    assertEq("Bad store addr", store.addr, address);
    assertEq("Bad store length", store.len, length);
    if(memcmp(store.bytes, data, length)) throw std::runtime_error("store bad data ???");
}

void CpuMemoryView::fetch(u32 address,u32 length, u8 *data){
    memory.read(address, length, data);
}

void CpuMemoryView::step(){
    if(loadFresh) throw std::runtime_error("loadFresh unused ???");
    if(storeFresh) throw std::runtime_error("storeFresh unused ???");
}
