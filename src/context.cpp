#include "context.hpp"

void Context::loadElf(std::string path, u64 offset){
    auto elf = new Elf(path.c_str());
    elf->visitBytes([&](u8 data, u64 address) {
        memory.write(address+offset, 1, &data);
    });
}


void Context::loadBin(std::string path, u64 offset){
    memory.loadBin(path, offset);
}

void Context::loadBytes(u64 offset, u32 length, u8* bytes){
    memory.write(offset, length, bytes);
}


void Context::cpuMemoryViewNew(u32 id, u64 readIds, u64 writeIds){
    cpuMemoryViews.try_emplace(id, *this, memory, readIds, writeIds);
}

void Context::rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth, u32 pmpNum, u32 triggerCount, u32 asidWidth, u32 viewId, FILE *logs){
    auto viewIter = cpuMemoryViews.find(viewId);
    auto& view = viewIter->second;
    view.bindHartId(hartId);
    auto [iter, inserted] = harts.try_emplace(hartId, *this, hartId, isa, priv, physWidth, pmpNum, triggerCount, asidWidth, &view, logs);
    if (!inserted) return;
    auto &hart = iter->second;
    if(config.spikeDebug) hart.proc->debug = true;
    if(config.spikeLogCommit) hart.proc->enable_log_commits();
}

void Context::close(){
    for(auto& [hartId, hart]: harts){
        hart.close();
    }
}

void Context::print(){
    if(time != 0xFFFFFFFFFFFFFFFF){
        printf("time >= %ld\n", time);
    }
}
