#pragma once

#include "type.h"
#include "memory.hpp"
#include "hart.hpp"
#include "elf.hpp"
#include <vector>
#include <string>
#include "coherency.hpp"

class Context{
public:
    Memory memory;
    std::vector<Hart*> harts;
    std::vector<CpuMemoryView*> cpuMemoryViews;
    FILE *spikeLogs;

    void loadElf(std::string path, u64 offset);
    void loadBin(std::string path, u64 offset);
    void cpuMemoryViewNew(u32 id, u64 readIds, u64 writeIds);
    void rvNew(u32 hartId, std::string isa, std::string priv, u32 physWidth, u32 viewId, FILE * logs);
    void close();
};

