#include "coherency.hpp"
#include <string.h>
#include "global.hpp"

CpuMemoryView::CpuMemoryView(Memory &memory, u64 readIds, u64 writeIds) : memory(memory){
    loadsInflightCount = 0;
    loadsInflight.resize(readIds);
    storeHead = NULL;
	storeLast = NULL;
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
    memory.read(addr, len, load.bytes);
    Access *store = storeHead;
    while(store){
    	if(!store->valid)
			throw std::runtime_error("store wasn't valid wuuut ???");
    	store->bypass(load);
    	store = store->next;
    }
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

void CpuMemoryView::storeExecute(u64 id, u64 addr, size_t len, const u8* bytes){
    auto &store = stores[id];
//    if(store.valid) throw std::runtime_error("Store was valid ???");
    if(len > 8) throw std::runtime_error("Store len to big ???");
    if(store.commited) throw std::runtime_error("Store was already commited ???");
    if(store.broadcasted) throw std::runtime_error("Store was already broadcasted ???");
    store.executed = true;
    store.addr = addr;
    store.len = len;
    memcpy(store.bytes, bytes, len);
}

void CpuMemoryView::storeCommit(u64 id){
    auto &store = stores[id];
    if(storeFresh) throw std::runtime_error("storeFresh was valid ???");
    storeFresh = &store;
    if(!store.executed) throw std::runtime_error("Store wasn't executed ???");
    if(store.commited) throw std::runtime_error("Store was already commited ???");
    store.commited = true;

    if(!store.broadcasted){
		store.valid = true;
		store.previous = storeLast;
		store.next = NULL;
		if(storeLast) {
			storeLast->next = &store;
		} else {
			storeHead = &store;
		}
		storeLast = &store;
    } else {
    	store.clear();
    }
}

void CpuMemoryView::storeBroadcast(u64 id){
    auto &store = stores[id];
    if(!store.executed) throw std::runtime_error("Store wasn't executed ???");
    if(store.broadcasted) throw std::runtime_error("Store was already broadcasted ???");
    memory.write(store.addr, store.len, store.bytes);
    store.broadcasted = true;
    if(store.commited){
		if(store.next){
			store.next->previous = store.previous;
		} else {
			storeLast = store.previous;
		}
		if(store.previous){
			store.previous->next = store.next;
		} else {
			storeHead = store.next;
		}
		store.valid = false;
    	store.clear();
    }
}


//Spike interface
void CpuMemoryView::load(u64 address,u32 length, u8 *data){
    if(!loadFresh)
        throw std::runtime_error("loadFresh wasn't NULL on check ???");
    if(!loadFresh->valid)
        throw std::runtime_error("load wasn't valid on check ???");
    auto &load = *loadFresh; loadFresh = NULL;
    assertEq("Bad load addr", load.addr, address);
    assertEq("Bad load length", load.len, length);
    memcpy(data, load.bytes, load.len);
    load.valid = false;
}

void CpuMemoryView::store(u64 address,u32 length, const u8 *data){
    if(!storeFresh) throw std::runtime_error("storeFresh wasn't valid on check ???");
    //if(!storeFresh->executed) throw std::runtime_error("Store wasn't executed on check ???");
    auto &store = *storeFresh; storeFresh = NULL;
    assertEq("Bad store addr", store.addr, address);
    assertEq("Bad store length", store.len, length);
    if(memcmp(store.bytes, data, length)) {
        printf("Got, expected:\n");
        for(u32 i = 0; i < length; ++i)
            printf("%02X ", store.bytes[i]);
        printf("\n");
        for(u32 i = 0; i < length; ++i)
            printf("%02X ", data[i]);
        printf("\n");
        throw std::runtime_error("store bad data ???");
    }

    //Bypass store values to inflight loads
    for(u64 i = 0; i < loadsInflightCount;i++){
        auto &load = *loadsInflight[i];
        if(!load.valid)
            throw std::runtime_error("load wasn't valid wuuut ???");
        store.bypass(load);
    }
}

void CpuMemoryView::fetch(u64 address,u32 length, u8 *data){
    memory.read(address, length, data);
}

void CpuMemoryView::mmu(u64 address,u32 length, u8 *data){
    memory.read(address, length, data);
}

void CpuMemoryView::step(){
    if(loadFresh) throw std::runtime_error("loadFresh unused ???");
    if(storeFresh) throw std::runtime_error("storeFresh unused ???");
}
