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

void Context::rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth){
    harts.resize(max((size_t)(hartId+1), harts.size()));
    harts[hartId] = new Hart(hartId, isa, priv, physWidth, &memory);
}

void Context::close(){
    for(auto hart : harts){
        if(hart) hart->close();
    }
}
