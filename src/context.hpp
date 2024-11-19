#pragma once

#include "type.h"
#include "memory.hpp"
#include "hart.hpp"
#include "elf.hpp"
#include <vector>
#include <string>
#include "coherency.hpp"
#include "config.hpp"

class Context{
public:
    RvlsConfig config;
    Memory memory;
    std::vector<Hart*> harts;
    std::vector<CpuMemoryView*> cpuMemoryViews;
    FILE *spikeLogs;
    u64 time = 0xFFFFFFFFFFFFFFFF;
    std::string lastErrorMessage;

    void loadElf(std::string path, u64 offset);
    void loadBin(std::string path, u64 offset);
    void loadBytes(u64 offset, u32 length, u8* bytes);
    void cpuMemoryViewNew(u32 id, u64 readIds, u64 writeIds);
    void rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth, u32 pmpNum, u32 viewId, FILE * logs);
    void close();
    void print();
};

