#include "context.hpp"

void Context::loadU32(u64 address, u32 data){
    memory.write(address, 4, (uint8_t*)&data);
}


void Context::loadElf(std::string path, u64 offset){
    auto elf = new Elf(path.c_str());
    elf->visitBytes([&](u8 data, u64 address) {
        memory.write(address+offset, 1, &data);
    });
}


void Context::loadBin(std::string path, u64 offset){
    memory.loadBin(path, offset);
}

void Context::cpuMemoryViewNew(u32 id, u64 readIds, u64 writeIds){
    cpuMemoryViews.resize(max((size_t)(id+1), cpuMemoryViews.size()));
    cpuMemoryViews[id] = new CpuMemoryView(memory, readIds, writeIds);
}

void Context::rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth, u32 viewId, FILE *logs){
    auto hart = new Hart(hartId, isa, priv, physWidth, cpuMemoryViews[viewId], logs);
    harts.resize(max((size_t)(hartId+1), harts.size()));
    harts[hartId] = hart;
    if(config.spikeDebug) hart->proc->debug = true;
    if(config.spikeLogCommit) hart->proc->enable_log_commits();
}

void Context::close(){
    for(auto hart : harts){
        if(hart) hart->close();
    }
}

void Context::print(){
    if(time != 0xFFFFFFFFFFFFFFFF){
        printf("time >= %ld\n", time);
    }
}
